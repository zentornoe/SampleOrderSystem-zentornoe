#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <queue>
#include "src/controller/OrderController.h"
#include "src/model/Order.h"
#include "src/model/Sample.h"
#include "src/model/OrderStatus.h"
#include "src/model/ProductionJob.h"
#include "test/mock/MockOrderRepository.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

namespace {
	const std::string S_ID = "S-001";
}

// ============================================================
//  공통 Fixture
// ============================================================
class StockAwareApproveTest : public ::testing::Test {
protected:
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	std::unique_ptr<OrderController> ctrl;

	void SetUp() override {
		ctrl = std::make_unique<OrderController>(mockOrderRepo, mockSampleRepo, prodQueue);
	}

	Order makeReservedOrder(const std::string& orderId, int qty) {
		return Order(orderId, S_ID, "고객사", qty);
	}

	Sample makeSample(int stock, int reservedQty = 0) {
		return Sample(S_ID, "시료A", 10.0, 0.9, stock, reservedQty);
	}
};

// ============================================================
// TC 1: 생산 진척도(60%) 반영 가용재고가 주문 수량과 같을 때 CONFIRMED
// Job: actualProd=200, totalSec=120000. 60%(72000초) → currentProd=120.
// stock=0, reservedQty=100. effective=120, available=20. qty=20 → CONFIRMED
// ============================================================
TEST_F(StockAwareApproveTest,
       ApproveOrder_JobInProgress_EffectiveStockSufficient_Confirmed)
{
	// actualProd = ceil(162 / (0.9*0.9)) = 200
	ProductionJob jobA("ORD-A", S_ID, 162, 0.9, 10.0);
	long long nowSec = jobA.getStartTime() + 72000LL; // 60% → currentProd=120
	prodQueue.push(jobA);

	Order  inOrder       = makeReservedOrder("ORD-B", 20);
	Sample inSample      = makeSample(0, 100);
	Order  capturedOrder = inOrder;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-B")).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(S_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).Times(1);
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));

	ctrl->approveOrder("ORD-B", nowSec);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
	EXPECT_EQ(prodQueue.size(), 1u); // Job A 유지, 신규 Job 없음
}

// ============================================================
// TC 2: 진척도 반영해도 가용재고 부족 시 PRODUCING + reservedQty 증가
// Job 60% → effective=120, available=20. qty=21 > 20 → PRODUCING
// capturedSample.reservedQty == 100+21 = 121
// ============================================================
TEST_F(StockAwareApproveTest,
       ApproveOrder_JobInProgress_EffectiveStockInsufficient_Producing)
{
	ProductionJob jobA("ORD-A", S_ID, 162, 0.9, 10.0);
	long long nowSec = jobA.getStartTime() + 72000LL;
	prodQueue.push(jobA);

	Order  inOrder        = makeReservedOrder("ORD-B", 21);
	Sample inSample       = makeSample(0, 100);
	Order  capturedOrder  = inOrder;
	Sample capturedSample = inSample;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-B")).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(S_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));

	ctrl->approveOrder("ORD-B", nowSec);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::PRODUCING);
	EXPECT_EQ(prodQueue.size(), 2u);
	EXPECT_EQ(capturedSample.getReservedQty(), 121);
}

// ============================================================
// TC 3: PRODUCING 경로에서 sampleRepo.update 호출 및 reservedQty 증가 검증
// stock=10, reservedQty=0, qty=100. 큐 없음. available=10 < 100 → PRODUCING.
// capturedSample.reservedQty == 100
// ============================================================
TEST_F(StockAwareApproveTest,
       ApproveOrder_ProducingPath_SampleRepoUpdatedWithReservedQty)
{
	Order  inOrder        = makeReservedOrder("ORD-A", 100);
	Sample inSample       = makeSample(10, 0);
	Sample capturedSample = inSample;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-A")).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(S_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);

	ctrl->approveOrder("ORD-A", 0LL);

	EXPECT_EQ(capturedSample.getReservedQty(), 100);
}

// ============================================================
// TC 4: 기존 예약 수량이 있을 때 가용재고(stock - reservedQty) 기준 CONFIRM 결정
// stock=50, reservedQty=30. available=20. qty=20 → CONFIRMED
// ============================================================
TEST_F(StockAwareApproveTest,
       ApproveOrder_ExistingReservations_AvailableStockDeterminesDecision)
{
	Order  inOrder       = makeReservedOrder("ORD-A", 20);
	Sample inSample      = makeSample(50, 30);
	Order  capturedOrder = inOrder;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-A")).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(S_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).Times(1);
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));

	ctrl->approveOrder("ORD-A", 0LL);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
}

// ============================================================
// TC 5: PRODUCING 경로 shortage가 가용재고 기준으로 계산된다
// stock=30, reservedQty=100. 큐 없음. effective=30, available=-70 → max=0.
// qty=50 → shortage = 50 - 0 = 50. actualProd = ceil(50/0.81) = 62
// ============================================================
TEST_F(StockAwareApproveTest,
       ApproveOrder_ProducingPath_ShortageBasedOnAvailableNotBaseStock)
{
	Order  inOrder  = makeReservedOrder("ORD-B", 50);
	Sample inSample = makeSample(30, 100);

	EXPECT_CALL(mockOrderRepo,  findById("ORD-B")).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(S_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).Times(1);
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);

	ctrl->approveOrder("ORD-B", 0LL);

	ASSERT_EQ(prodQueue.size(), 1u);
	EXPECT_EQ(prodQueue.front().getShortage(),   50);
	EXPECT_EQ(prodQueue.front().getActualProd(), 62);
}