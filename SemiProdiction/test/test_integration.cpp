#include <gtest/gtest.h>
#include <filesystem>
#include <queue>
#include <memory>
#include "src/repository/JsonHelper.h"
#include "src/repository/JsonSampleRepository.h"
#include "src/repository/JsonOrderRepository.h"
#include "src/controller/SampleController.h"
#include "src/controller/OrderController.h"
#include "src/controller/ProductionController.h"
#include "src/controller/ReleaseController.h"
#include "src/model/OrderStatus.h"
#include "src/model/ProductionJob.h"

class IntegrationTest : public ::testing::Test {
protected:
	std::filesystem::path samplePath;
	std::filesystem::path orderPath;

	std::unique_ptr<JsonSampleRepository> sampleRepo;
	std::unique_ptr<JsonOrderRepository>  orderRepo;
	std::queue<ProductionJob>             prodQueue;

	std::unique_ptr<SampleController>     sampleCtrl;
	std::unique_ptr<OrderController>      orderCtrl;
	std::unique_ptr<ProductionController> prodCtrl;
	std::unique_ptr<ReleaseController>    releaseCtrl;

	void SetUp() override {
		auto tmp = std::filesystem::temp_directory_path();
		samplePath = tmp / "it_samples.json";
		orderPath  = tmp / "it_orders.json";

		jsonWriteFile(samplePath.string(), "[]");
		jsonWriteFile(orderPath.string(),  "[]");

		sampleRepo  = std::make_unique<JsonSampleRepository>(samplePath.string());
		orderRepo   = std::make_unique<JsonOrderRepository>(orderPath.string());
		sampleCtrl  = std::make_unique<SampleController>(*sampleRepo);
		orderCtrl   = std::make_unique<OrderController>(*orderRepo, *sampleRepo, prodQueue);
		prodCtrl    = std::make_unique<ProductionController>(*orderRepo, *sampleRepo, prodQueue);
		releaseCtrl = std::make_unique<ReleaseController>(*orderRepo, *sampleRepo);
	}

	void TearDown() override {
		std::filesystem::remove(samplePath);
		std::filesystem::remove(orderPath);
	}
};

// TC1: 재고 충분 - 주문 접수 -> 승인 -> CONFIRMED -> 출고 -> RELEASE
TEST_F(IntegrationTest, SufficientStock_ReserveApproveRelease) {
	// GIVEN: 시료 S-001 (stock=200, yield=0.92, avgProdTime=5.0)
	sampleCtrl->registerSample("S-001", "TestSample", 5.0, 0.92, 200);

	// WHEN: 주문 접수 (qty=100)
	std::string orderId = orderCtrl->reserveOrder("S-001", "Customer-A", 100);
	ASSERT_FALSE(orderId.empty());

	// WHEN: 승인 (재고 충분 -> CONFIRMED)
	orderCtrl->approveOrder(orderId, 0LL);

	Order order = orderCtrl->getOrder(orderId);
	EXPECT_EQ(order.getStatus(), OrderStatus::CONFIRMED);

	Sample sample = sampleCtrl->getSample("S-001");
	EXPECT_EQ(sample.getReservedQty(), 100);

	// WHEN: 출고
	releaseCtrl->releaseOrder(orderId);

	// THEN: RELEASE, stock==100, reservedQty==0
	Order released = orderCtrl->getOrder(orderId);
	EXPECT_EQ(released.getStatus(), OrderStatus::RELEASE);

	Sample afterRelease = sampleCtrl->getSample("S-001");
	EXPECT_EQ(afterRelease.getStock(), 100);
	EXPECT_EQ(afterRelease.getReservedQty(), 0);
}

// TC2: 재고 부족 - 주문 접수 -> 승인 -> PRODUCING -> tick 완료 -> CONFIRMED -> 출고 -> RELEASE
// shortage=70, actualProd=ceil(70/(0.9*0.9))=ceil(86.42)=87
// completionTime = 0 + (long long)(87 * 0.001 * 60) = 5
TEST_F(IntegrationTest, InsufficientStock_ProductionThenRelease) {
	// GIVEN: 시료 S-001 (stock=30, yield=0.9, avgProdTime=0.001)
	sampleCtrl->registerSample("S-001", "TestSample", 0.001, 0.9, 30);

	// WHEN: 주문 접수 (qty=100)
	std::string orderId = orderCtrl->reserveOrder("S-001", "Customer-B", 100);
	ASSERT_FALSE(orderId.empty());

	// WHEN: 승인 (재고 부족 -> PRODUCING)
	orderCtrl->approveOrder(orderId, 0LL);

	Order producing = orderCtrl->getOrder(orderId);
	EXPECT_EQ(producing.getStatus(), OrderStatus::PRODUCING);

	Sample reservedSample = sampleCtrl->getSample("S-001");
	EXPECT_EQ(reservedSample.getReservedQty(), 100);

	// 생산 완료 시각 가져오기
	ASSERT_TRUE(prodCtrl->hasJob());
	ProductionJob job = prodCtrl->peekNextJob();
	long long completionTime = job.getCompletionTime();
	int actualProd = job.getActualProd();

	// actualProd = ceil(70/(0.9*0.9)) = 87
	EXPECT_EQ(actualProd, 87);

	// WHEN: tick으로 생산 완료 -> CONFIRMED
	prodCtrl->tick(completionTime);

	Order confirmed = orderCtrl->getOrder(orderId);
	EXPECT_EQ(confirmed.getStatus(), OrderStatus::CONFIRMED);

	// stock = 30 + 87 = 117, reservedQty = 100
	Sample afterProd = sampleCtrl->getSample("S-001");
	EXPECT_EQ(afterProd.getStock(), 117);
	EXPECT_EQ(afterProd.getReservedQty(), 100);

	// WHEN: 출고
	releaseCtrl->releaseOrder(orderId);

	// THEN: RELEASE, stock=117-100=17, reservedQty=0
	Order released = orderCtrl->getOrder(orderId);
	EXPECT_EQ(released.getStatus(), OrderStatus::RELEASE);

	Sample afterRelease = sampleCtrl->getSample("S-001");
	EXPECT_EQ(afterRelease.getStock(), 17);
	EXPECT_EQ(afterRelease.getReservedQty(), 0);
}

// TC3: 주문 거절 - 주문 접수 -> 거절 -> REJECTED, 재고 변동 없음
TEST_F(IntegrationTest, RejectOrder_StockUnchanged_QueueEmpty) {
	// GIVEN: 시료 S-001 (stock=200)
	sampleCtrl->registerSample("S-001", "TestSample", 5.0, 0.92, 200);

	// WHEN: 주문 접수 (qty=50)
	std::string orderId = orderCtrl->reserveOrder("S-001", "Customer-C", 50);
	ASSERT_FALSE(orderId.empty());

	// WHEN: 거절
	orderCtrl->rejectOrder(orderId);

	// THEN: REJECTED
	Order rejected = orderCtrl->getOrder(orderId);
	EXPECT_EQ(rejected.getStatus(), OrderStatus::REJECTED);

	// 재고 변동 없음
	Sample sample = sampleCtrl->getSample("S-001");
	EXPECT_EQ(sample.getStock(), 200);

	// 생산 큐 비어 있음
	EXPECT_TRUE(prodQueue.empty());
}

// TC4: FIFO 다중 생산 - 두 주문 모두 PRODUCING -> 순서대로 tick 완료
// S-001 stock=0
// 주문1 qty=50: shortage=50, actualProd=ceil(50/(0.9*0.9))=ceil(61.72)=62
// 주문2 qty=30: shortage=30, actualProd=ceil(30/(0.9*0.9))=ceil(37.03)=38
TEST_F(IntegrationTest, FifoMultiProduction_BothConfirmedInOrder) {
	// GIVEN: 시료 S-001 (stock=0, yield=0.9, avgProdTime=0.001)
	sampleCtrl->registerSample("S-001", "TestSample", 0.001, 0.9, 0);

	// WHEN: 주문 2건 접수 및 승인
	std::string orderId1 = orderCtrl->reserveOrder("S-001", "Customer-D", 50);
	std::string orderId2 = orderCtrl->reserveOrder("S-001", "Customer-E", 30);
	ASSERT_FALSE(orderId1.empty());
	ASSERT_FALSE(orderId2.empty());

	orderCtrl->approveOrder(orderId1, 0LL);
	orderCtrl->approveOrder(orderId2, 0LL);

	// 두 주문 모두 PRODUCING
	EXPECT_EQ(orderCtrl->getOrder(orderId1).getStatus(), OrderStatus::PRODUCING);
	EXPECT_EQ(orderCtrl->getOrder(orderId2).getStatus(), OrderStatus::PRODUCING);

	// 생산 큐에 2건
	EXPECT_EQ(prodCtrl->getQueue().size(), static_cast<size_t>(2));

	// WHEN: 첫 번째 작업 완료 시간 저장 후 tick
	ASSERT_TRUE(prodCtrl->hasJob());
	long long completionTime1 = prodCtrl->peekNextJob().getCompletionTime();

	prodCtrl->tick(completionTime1);

	// THEN: orderId1 -> CONFIRMED, 큐에 1건 남음
	EXPECT_EQ(orderCtrl->getOrder(orderId1).getStatus(), OrderStatus::CONFIRMED);
	EXPECT_EQ(prodCtrl->getQueue().size(), static_cast<size_t>(1));

	// WHEN: 두 번째 작업 완료 시간 저장 후 tick
	ASSERT_TRUE(prodCtrl->hasJob());
	long long completionTime2 = prodCtrl->peekNextJob().getCompletionTime();

	prodCtrl->tick(completionTime2);

	// THEN: orderId2 -> CONFIRMED, 큐 비어 있음
	EXPECT_EQ(orderCtrl->getOrder(orderId2).getStatus(), OrderStatus::CONFIRMED);
	EXPECT_FALSE(prodCtrl->hasJob());
}