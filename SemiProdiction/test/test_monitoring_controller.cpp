#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include "src/controller/MonitoringController.h"
#include "src/model/Order.h"
#include "src/model/Sample.h"
#include "src/model/OrderStatus.h"
#include "src/model/MonitoringSummary.h"
#include "test/mock/MockOrderRepository.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::_;

// ============================================================
//  공통 Fixture - Mock + Controller 셋업
// ============================================================
class MonitoringControllerTest : public ::testing::Test {
protected:
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::unique_ptr<MonitoringController> ctrl;

	void SetUp() override {
		ctrl = std::make_unique<MonitoringController>(mockOrderRepo, mockSampleRepo);
	}
};

// 테스트 1: REJECTED 1건 제외한 5건(RESERVED 1, CONFIRMED 2, PRODUCING 1, RELEASE 1) 집계
TEST_F(MonitoringControllerTest, GetOrderSummary_SixOrders_RejectsExcludedFromCount)
{
	std::vector<Order> orders = {
		Order("ORD-001", "S-001", "삼성전자",  100),
		Order("ORD-002", "S-001", "SK하이닉스", 50,  OrderStatus::CONFIRMED,  0LL),
		Order("ORD-003", "S-001", "LG전자",     80,  OrderStatus::CONFIRMED,  0LL),
		Order("ORD-004", "S-001", "현대전자",   60,  OrderStatus::PRODUCING,  0LL),
		Order("ORD-005", "S-001", "인텔코리아", 120, OrderStatus::RELEASE,    0LL),
		Order("ORD-006", "S-001", "마이크론",   200, OrderStatus::REJECTED,   0LL)
	};

	EXPECT_CALL(mockOrderRepo, findAll()).WillOnce(Return(orders));

	OrderSummary summary = ctrl->getOrderSummary();

	EXPECT_EQ(summary.totalReserved,  1);
	EXPECT_EQ(summary.totalConfirmed, 2);
	EXPECT_EQ(summary.totalProducing, 1);
	EXPECT_EQ(summary.totalRelease,   1);
}

// 테스트 2: stock == reservedQty 경계값 → SUFFICIENT
TEST_F(MonitoringControllerTest, GetStockStatus_StockEqualsReservedQty_ReturnsSufficient)
{
	Sample sample("S-001", "시료A", 5.0, 0.92, 100);

	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample));

	StockStatus status = ctrl->getStockStatus("S-001", 100);

	EXPECT_EQ(status, StockStatus::SUFFICIENT);
}

// 테스트 3: 0 < stock < reservedQty → SHORTAGE
TEST_F(MonitoringControllerTest, GetStockStatus_StockLessThanReservedQty_ReturnsShortage)
{
	Sample sample("S-001", "시료A", 5.0, 0.92, 30);

	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample));

	StockStatus status = ctrl->getStockStatus("S-001", 100);

	EXPECT_EQ(status, StockStatus::SHORTAGE);
}

// 테스트 4: stock == 0 → DEPLETED (reservedQty에 무관)
TEST_F(MonitoringControllerTest, GetStockStatus_StockZero_ReturnsDepleted)
{
	Sample sample("S-001", "시료A", 5.0, 0.92, 0);

	EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(sample));

	StockStatus status = ctrl->getStockStatus("S-001", 50);

	EXPECT_EQ(status, StockStatus::DEPLETED);
}

// 테스트 5: REJECTED 제외한 활성 주문 3건 반환
TEST_F(MonitoringControllerTest, GetActiveOrders_FourOrders_ReturnsThreeExcludingRejected)
{
	std::vector<Order> orders = {
		Order("ORD-001", "S-001", "삼성전자",  100),
		Order("ORD-002", "S-001", "SK하이닉스", 50,  OrderStatus::CONFIRMED, 0LL),
		Order("ORD-003", "S-001", "LG전자",     80,  OrderStatus::PRODUCING, 0LL),
		Order("ORD-004", "S-001", "현대전자",   60,  OrderStatus::REJECTED,  0LL)
	};

	EXPECT_CALL(mockOrderRepo, findAll()).WillOnce(Return(orders));

	std::vector<Order> result = ctrl->getActiveOrders();

	EXPECT_EQ(result.size(), 3u);
	for (const auto& order : result) {
		EXPECT_NE(order.getStatus(), OrderStatus::REJECTED);
	}
}