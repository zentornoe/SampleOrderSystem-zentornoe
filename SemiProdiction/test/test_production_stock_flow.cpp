#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <queue>
#include <memory>
#include "src/controller/OrderController.h"
#include "src/controller/ProductionController.h"
#include "src/controller/ReleaseController.h"
#include "src/model/OrderStatus.h"
#include "src/model/ProductionJob.h"
#include "test/mock/MockOrderRepository.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

// ============================================================
//  공통 Fixture — 세 Controller 가 동일한 prodQueue 를 공유
// ============================================================
class ProductionStockFlowTest : public ::testing::Test {
protected:
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;

	std::unique_ptr<OrderController>      orderCtrl;
	std::unique_ptr<ProductionController> prodCtrl;
	std::unique_ptr<ReleaseController>    releaseCtrl;

	void SetUp() override {
		orderCtrl   = std::make_unique<OrderController>(mockOrderRepo, mockSampleRepo, prodQueue);
		prodCtrl    = std::make_unique<ProductionController>(prodQueue, mockOrderRepo, mockSampleRepo);
		releaseCtrl = std::make_unique<ReleaseController>(mockOrderRepo, mockSampleRepo);
	}

	// shortage=70, yield=0.9 → actualProd = ceil(70 / (0.9*0.9)) = ceil(86.42) = 87
	ProductionJob makeJob(const std::string& orderId = "ORD-001",
	                      const std::string& sampleId = "S-001",
	                      double avgProdTime = 0.001)
	{
		return ProductionJob(orderId, sampleId, 70, 0.9, avgProdTime);
	}

	Order makeOrder(const std::string& orderId, OrderStatus status, int qty = 100) {
		return Order(orderId, "S-001", "고객", qty, status, 0LL);
	}

	Sample makeSample(int stock, int reservedQty = 0) {
		return Sample("S-001", "시료", 5.0, 0.9, stock, reservedQty);
	}
};

// ============================================================
//  [메인 시나리오] 생산 완료 시 중간 출고를 반영한 실재고에 생산량을 더함
//  흐름: 초기 재고 30 → 생산 중 5개 출고 → 재고 25 → 생산 완료(+87) → 재고 112
// ============================================================
TEST_F(ProductionStockFlowTest,
       Tick_WhenIntermediateReleaseOccurred_AddsActualProdToCurrentStock)
{
	// avgProdTime=0.001, actualProd=87 → completionTime = startTime + 5
	ProductionJob job = makeJob();
	long long completionTime = job.getCompletionTime();
	prodQueue.push(job);

	// 생산 완료 시점에 중간 출고로 stock 이 25 로 감소한 상태
	Order  inOrder  = makeOrder("ORD-001", OrderStatus::PRODUCING);
	Sample inSample = makeSample(25); // 초기 30에서 5개 출고 반영

	Sample capturedSample = inSample;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-001")).WillOnce(Return(inOrder));
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));

	prodCtrl->tick(completionTime);

	// 중간 출고(5) 반영된 25 + actualProd(87) = 112
	EXPECT_EQ(capturedSample.getStock(), 25 + job.getActualProd());
	EXPECT_EQ(capturedSample.getStock(), 112);
}

// ============================================================
//  [메인 시나리오] 생산 시작 시각 기준 진척도 반환
//  shortage=70, yield=0.9, avgProdTime=60.0
//  actualProd=87, totalProdTime=60*87=5220min=313200sec
// ============================================================
TEST_F(ProductionStockFlowTest,
       GetProgress_AtVariousTimePoints_ReturnsCorrectProgress)
{
	ProductionJob job = makeJob("ORD-001", "S-001", 60.0);
	// actualProd=87, totalProdSec=313200
	long long startTime = job.getStartTime();
	long long totalSec  = 313200LL;
	prodQueue.push(job);

	// 시작 시점: 0%
	EXPECT_DOUBLE_EQ(prodCtrl->getProgress(startTime), 0.0);

	// 절반 경과: elapsed=156600 → currentProd=(int)(87*0.5)=43 → 43/87*100≈49.4%
	EXPECT_NEAR(prodCtrl->getProgress(startTime + totalSec / 2), 50.0, 1.5);

	// 완료 시각 이후: 100%
	EXPECT_DOUBLE_EQ(prodCtrl->getProgress(startTime + totalSec), 100.0);
}

// ============================================================
//  [메인 시나리오] 생산 중 신규 소량 주문은 실재고(30)로 즉시 CONFIRMED
//  prodQueue 에 Job1 이 있어도 approveOrder 는 sample.getStock() 기준으로 판단
// ============================================================
TEST_F(ProductionStockFlowTest,
       ApproveOrder_SmallNewOrderDuringProduction_ConfirmedFromBaseStock)
{
	// Job1 이 생산 중 (prodQueue 에 적재된 상태)
	prodQueue.push(makeJob("ORD-001"));

	// 신규 주문 ORD-002 (qty=5) 승인
	Order  inOrder2 = makeOrder("ORD-002", OrderStatus::RESERVED, 5);
	Sample inSample = makeSample(30); // 실재고 30, 생산 진척 무관

	Order  capturedOrder  = inOrder2;
	Sample capturedSample = inSample;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-002")).WillOnce(Return(inOrder2));
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(inSample));
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));

	orderCtrl->approveOrder("ORD-002");

	// 실재고 30 >= 5 → CONFIRMED, reservedQty 5 증가
	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
	EXPECT_EQ(capturedSample.getReservedQty(), 5);
	// Job1 은 여전히 큐에 대기
	EXPECT_EQ(prodQueue.size(), 1u);
}

// ============================================================
//  [메인 시나리오] 생산 완료 후 기존 주문(100) + 중간 출고된 소량 주문(5) 모두 처리
//  완료 후 재고 112, Order1 출고(−100) → 12, 5개짜리는 이미 출고됨
// ============================================================
TEST_F(ProductionStockFlowTest,
       AfterProduction_OriginalAndIntermediateOrderBothReleasable)
{
	// 생산 완료 후 재고 = 112 (25 + 87, 중간 출고 5 반영)
	// Order1(qty=100)이 CONFIRMED 상태, PRODUCING 시 reserveQty(100) 호출됨
	Order  confirmedOrder = makeOrder("ORD-001", OrderStatus::CONFIRMED, 100);
	Sample postProdSample = makeSample(112, 100);

	Sample capturedSample = postProdSample;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-001")).WillOnce(Return(confirmedOrder));
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(postProdSample));
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));

	releaseCtrl->releaseOrder("ORD-001");

	// 112 − 100 = 12 (잉여 재고)
	EXPECT_EQ(capturedSample.getStock(), 12);
}

// ============================================================
//  [추가 1] 두 Job 순차 완료 — 첫 번째 완료 후 출고 발생 시 두 번째는 감소된 재고에 더함
//  Job1: stock 30 + 87 = 117 → 5개 출고 → stock 112
//  Job2 완료: stock 112 + job2.actualProd
// ============================================================
TEST_F(ProductionStockFlowTest,
       TwoJobs_IntermediateReleaseBetweenCompletions_SecondJobAddsToReducedStock)
{
	// actualProd=87, avgProdTime=0.001 → completionTime = startTime + 5
	ProductionJob job1 = makeJob("ORD-001");
	// Job2: shortage=50, yield=0.9 → actualProd = ceil(50/0.81) = ceil(61.7) = 62
	ProductionJob job2("ORD-002", "S-001", 50, 0.9, 0.001);
	prodQueue.push(job1);
	prodQueue.push(job2);

	long long t = job1.getCompletionTime();

	// Job1 완료 시: stock=30
	Order  ord1    = makeOrder("ORD-001", OrderStatus::PRODUCING, 100);
	Sample sample1 = makeSample(30);
	// Job2 완료 시: 5개 중간 출고 후 stock=30+87-5=112
	Order  ord2    = makeOrder("ORD-002", OrderStatus::PRODUCING, 80);
	Sample sample2 = makeSample(112);

	Sample captured1 = sample1, captured2 = sample2;

	{
		::testing::InSequence seq;
		EXPECT_CALL(mockOrderRepo,  findById("ORD-001")).WillOnce(Return(ord1));
		EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(::testing::DoDefault());
		EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample1));
		EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&captured1));

		EXPECT_CALL(mockOrderRepo,  findById("ORD-002")).WillOnce(Return(ord2));
		EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(::testing::DoDefault());
		EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample2));
		EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&captured2));
	}

	prodCtrl->tick(t); // Job1 완료
	prodCtrl->tick(t); // Job2 완료

	EXPECT_EQ(captured1.getStock(), 30 + job1.getActualProd());   // 30+87=117
	EXPECT_EQ(captured2.getStock(), 112 + job2.getActualProd());  // 112+62=174
}

// ============================================================
//  [추가 2] 생산 잉여분(수율 안전계수 0.9)으로 소량 주문 연속 처리
//  완료 후: stock=112, Order1 출고(−100) → 12 잔여
//  5개짜리 주문 2건이 잉여 재고(12)에서 연속 출고 가능
// ============================================================
TEST_F(ProductionStockFlowTest,
       ProductionSurplus_MultipleSmallOrdersServedFromRemainingStock)
{
	// 생산 완료 후 재고=112. PRODUCING 시 reserveQty(100) 호출됨
	// Order1(qty=100) 출고 → stock=12, reservedQty=0 남음
	Order  ord1    = makeOrder("ORD-001", OrderStatus::CONFIRMED, 100);
	Sample sample1 = makeSample(112, 100);
	Sample captured1 = sample1;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-001")).WillOnce(Return(ord1));
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample1));
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&captured1));
	releaseCtrl->releaseOrder("ORD-001");
	EXPECT_EQ(captured1.getStock(), 12); // 잉여 12개

	// 잉여분에서 5개짜리 주문 1 출고
	Order  ord2    = makeOrder("ORD-002", OrderStatus::CONFIRMED, 5);
	Sample sample2 = makeSample(12, 5); // 5개 예약
	Sample captured2 = sample2;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-002")).WillOnce(Return(ord2));
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample2));
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&captured2));
	releaseCtrl->releaseOrder("ORD-002");
	EXPECT_EQ(captured2.getStock(), 7); // 12−5=7

	// 잉여분에서 5개짜리 주문 2 출고
	Order  ord3    = makeOrder("ORD-003", OrderStatus::CONFIRMED, 5);
	Sample sample3 = makeSample(7, 5);
	Sample captured3 = sample3;

	EXPECT_CALL(mockOrderRepo,  findById("ORD-003")).WillOnce(Return(ord3));
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample3));
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&captured3));
	releaseCtrl->releaseOrder("ORD-003");
	EXPECT_EQ(captured3.getStock(), 2); // 7−5=2
}

// ============================================================
//  [추가 3] 생산 완료 후 다중 CONFIRMED 주문 순차 출고 — 재고 단계적 차감 검증
//  stock=200 (생산 완료), 3건의 주문 순서대로 출고
// ============================================================
TEST_F(ProductionStockFlowTest,
       MultipleConfirmedOrders_SequentialRelease_StockDecrementsCorrectly)
{
	// 총 재고 200, 세 주문: 80, 70, 50
	auto runRelease = [&](const std::string& orderId, int qty,
	                      int stockBefore, int reservedQty) -> Sample {
		Order  ord    = makeOrder(orderId, OrderStatus::CONFIRMED, qty);
		Sample smpl   = makeSample(stockBefore, reservedQty);
		Sample captured = smpl;

		EXPECT_CALL(mockOrderRepo,  findById(orderId)).WillOnce(Return(ord));
		EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(smpl));
		EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);
		EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&captured));

		releaseCtrl->releaseOrder(orderId);
		return captured;
	};

	Sample after1 = runRelease("ORD-001", 80, 200, 80);
	EXPECT_EQ(after1.getStock(), 120); // 200−80

	Sample after2 = runRelease("ORD-002", 70, 120, 70);
	EXPECT_EQ(after2.getStock(), 50);  // 120−70

	Sample after3 = runRelease("ORD-003", 50, 50, 50);
	EXPECT_EQ(after3.getStock(), 0);   // 50−50
}