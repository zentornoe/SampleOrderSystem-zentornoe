#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <ctime>
#include <queue>
#include <memory>
#include "src/controller/ProductionController.h"
#include "src/model/OrderStatus.h"
#include "test/mock/MockOrderRepository.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

// ---------------------------------------------------------------------------
// Fixture
// ---------------------------------------------------------------------------
class ProductionControllerTest : public ::testing::Test {
protected:
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	std::unique_ptr<ProductionController> ctrl;

	void SetUp() override {
		ctrl = std::make_unique<ProductionController>(
			mockOrderRepo, mockSampleRepo, prodQueue);
	}

	// avgProdTime=0.001 -> totalProdTime=0.002min -> (long long)(0.002*60)=0
	// -> completionTime == startTime -> tick(startTime) 즉시 완료
	// shortage=1, yield=0.92 -> actualProd = ceil(1/0.828) = 2
	ProductionJob makeInstantJob(const std::string& orderId,
								 const std::string& sampleId = "S-001")
	{
		return ProductionJob(orderId, sampleId, 1, 0.92, 0.001);
	}

	// OrderStatus::PRODUCING 상태 주문 객체 생성 헬퍼
	Order makeProducingOrder(const std::string& orderId,
							 const std::string& customerName = "고객")
	{
		// 마지막 인자(0LL)는 예약 생성 타임스탬프 — 테스트에서 의미 없는 더미 값
		return Order(orderId, "S-001", customerName, 10, OrderStatus::PRODUCING, 0LL);
	}
};

// ---------------------------------------------------------------------------
// 테스트 1: 초기 상태 — 큐가 비어 있고 진행률 0.0
// ---------------------------------------------------------------------------
TEST_F(ProductionControllerTest, InitialState_QueueIsEmpty) {
	EXPECT_FALSE(ctrl->hasJob());
	EXPECT_TRUE(ctrl->getQueue().empty());
	EXPECT_DOUBLE_EQ(ctrl->getProgress(static_cast<long long>(std::time(nullptr))), 0.0);
}

// ---------------------------------------------------------------------------
// 테스트 2: 작업 추가 시 hasJob() 참, peekNextJob() 이 올바른 orderId 반환
// ---------------------------------------------------------------------------
TEST_F(ProductionControllerTest, PushJob_HasJobAndPeekReturnsCorrectOrderId) {
	prodQueue.push(makeInstantJob("ORD-001"));

	EXPECT_TRUE(ctrl->hasJob());
	EXPECT_EQ(ctrl->peekNextJob().getOrderId(), "ORD-001");
}

// ---------------------------------------------------------------------------
// 테스트 3: FIFO — 3개 job 이 ORD-001, ORD-002, ORD-003 순서로 처리됨
// ---------------------------------------------------------------------------
TEST_F(ProductionControllerTest, Tick_ThreeJobs_ProcessedInFifoOrder) {
	ProductionJob job1 = makeInstantJob("ORD-001");
	ProductionJob job2 = makeInstantJob("ORD-002");
	ProductionJob job3 = makeInstantJob("ORD-003");
	prodQueue.push(job1);
	prodQueue.push(job2);
	prodQueue.push(job3);

	// avgProdTime=0.001 → completionTime==startTime, 임의 미래 시각으로 완료 판정
	long long futureTime = static_cast<long long>(std::time(nullptr)) + 1000000LL;

	Order ord1 = makeProducingOrder("ORD-001", "고객A");
	Order ord2 = makeProducingOrder("ORD-002", "고객B");
	Order ord3 = makeProducingOrder("ORD-003", "고객C");
	Sample smp("S-001", "시료", 5.0, 0.92, 0);

	EXPECT_CALL(mockOrderRepo,  findById("ORD-001")).WillOnce(Return(ord1));
	EXPECT_CALL(mockOrderRepo,  findById("ORD-002")).WillOnce(Return(ord2));
	EXPECT_CALL(mockOrderRepo,  findById("ORD-003")).WillOnce(Return(ord3));
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(3);
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillRepeatedly(Return(smp));
	EXPECT_CALL(mockSampleRepo, update(_)).Times(3);

	// 첫 tick: ORD-001 완료, 다음 대기는 ORD-002
	ctrl->tick(futureTime);
	ASSERT_TRUE(ctrl->hasJob());
	EXPECT_EQ(ctrl->peekNextJob().getOrderId(), "ORD-002");

	// 두 번째 tick: ORD-002 완료, 다음 대기는 ORD-003
	ctrl->tick(futureTime);
	ASSERT_TRUE(ctrl->hasJob());
	EXPECT_EQ(ctrl->peekNextJob().getOrderId(), "ORD-003");

	// 세 번째 tick: ORD-003 완료, 큐 비어있음
	ctrl->tick(futureTime);
	EXPECT_FALSE(ctrl->hasJob());
}

// ---------------------------------------------------------------------------
// 테스트 4: 완료 시 Order CONFIRMED 전환 + Sample 재고 actualProd 만큼 증가
// ---------------------------------------------------------------------------
TEST_F(ProductionControllerTest, Tick_CompletedJob_OrderConfirmedAndStockAdded) {
	// shortage=1, yield=0.92 -> actualProd = ceil(1/0.828) = 2
	ProductionJob job = makeInstantJob("ORD-001");
	long long completionTime = job.getStartTime(); // totalProdTime<1sec -> completionTime==startTime

	Order  inOrder ("ORD-001", "S-001", "고객", 10, OrderStatus::PRODUCING, 0LL);
	Sample inSample("S-001", "시료", 5.0, 0.92, 50);

	Order  capturedOrder  = inOrder;
	Sample capturedSample = inSample;

	prodQueue.push(job);

	EXPECT_CALL(mockOrderRepo,  findById("ORD-001")).WillOnce(Return(inOrder));
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));

	ctrl->tick(completionTime);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
	// 기존 재고 50 + actualProd 2 = 52
	EXPECT_EQ(capturedSample.getStock(), 50 + job.getActualProd());
	EXPECT_FALSE(ctrl->hasJob());
}

// ---------------------------------------------------------------------------
// 테스트 5: getProgress() — 시작(0%), 중간(50%), 완료 후(100%) 반환
// ---------------------------------------------------------------------------
TEST_F(ProductionControllerTest, GetProgress_ReturnsCorrectProgressRatio) {
	// shortage=1, yield=0.92, avgProdTime=60.0
	// actualProd = ceil(1/(0.92*0.9)) = 2
	// totalProdTime = 60 * 2 = 120 min = 7200 sec
	ProductionJob job("ORD-001", "S-001", 1, 0.92, 60.0);
	long long startTime = job.getStartTime();
	prodQueue.push(job);

	// elapsed=0 -> getCurrentProd=0 -> 0%
	EXPECT_DOUBLE_EQ(ctrl->getProgress(startTime), 0.0);

	// elapsed=3600, totalSec=7200 -> ratio=0.5 -> getCurrentProd=1 -> 50%
	EXPECT_DOUBLE_EQ(ctrl->getProgress(startTime + 3600LL), 50.0);

	// elapsed 매우 크면 getCurrentProd=actualProd -> 100%
	EXPECT_DOUBLE_EQ(ctrl->getProgress(startTime + 1000000LL), 100.0);
}

// ---------------------------------------------------------------------------
// 테스트 6: 빈 큐에서 tick() 호출 시 repository 접근 없음
// ---------------------------------------------------------------------------
TEST_F(ProductionControllerTest, Tick_EmptyQueue_NoRepositoryCalls) {
	EXPECT_CALL(mockOrderRepo,  findById(_)).Times(0);
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(0);
	EXPECT_CALL(mockSampleRepo, findById(_)).Times(0);
	EXPECT_CALL(mockSampleRepo, update(_)).Times(0);

	ctrl->tick(static_cast<long long>(std::time(nullptr)));
}