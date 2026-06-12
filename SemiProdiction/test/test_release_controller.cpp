#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include "src/controller/ReleaseController.h"
#include "src/model/Order.h"
#include "src/model/Sample.h"
#include "src/model/OrderStatus.h"
#include "test/mock/MockOrderRepository.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

namespace {
	const std::string TEST_ORDER_ID  = "ORD-20260612-0001";
	const std::string TEST_SAMPLE_ID = "S-001";
}

class ReleaseControllerTest : public ::testing::Test {
protected:
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::unique_ptr<ReleaseController> ctrl;

	void SetUp() override {
		ctrl = std::make_unique<ReleaseController>(mockOrderRepo, mockSampleRepo);
	}

	Order makeConfirmedOrder(int quantity = 150) {
		return Order(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", quantity,
					 OrderStatus::CONFIRMED, 0LL);
	}

	Sample makeSample(int stock, int reservedQty = 0) {
		return Sample(TEST_SAMPLE_ID, "시료A", 5.0, 0.92, stock, reservedQty);
	}
};

// 테스트 1: 출고 후 주문 상태가 RELEASE로 전환된다
TEST_F(ReleaseControllerTest, ReleaseOrder_ConfirmedOrder_StatusChangesToRelease)
{
	Order capturedOrder = Order("dummy", "S-000", "dummy", 1);

	EXPECT_CALL(mockOrderRepo, findById(TEST_ORDER_ID))
		.WillOnce(Return(makeConfirmedOrder(150)));
	EXPECT_CALL(mockSampleRepo, findById(TEST_SAMPLE_ID))
		.WillOnce(Return(makeSample(300, 150)));
	EXPECT_CALL(mockOrderRepo, update(_))
		.WillOnce(SaveArg<0>(&capturedOrder));
	EXPECT_CALL(mockSampleRepo, update(_))
		.Times(1);

	ctrl->releaseOrder(TEST_ORDER_ID);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::RELEASE);
}

// 테스트 2: 출고 후 재고가 주문 수량만큼 정확히 차감된다 (300 → 150)
TEST_F(ReleaseControllerTest, ReleaseOrder_ConfirmedOrder_StockReducedByOrderQuantity)
{
	Sample capturedSample = makeSample(0);

	EXPECT_CALL(mockOrderRepo, findById(TEST_ORDER_ID))
		.WillOnce(Return(makeConfirmedOrder(150)));
	EXPECT_CALL(mockSampleRepo, findById(TEST_SAMPLE_ID))
		.WillOnce(Return(makeSample(300, 150)));
	EXPECT_CALL(mockOrderRepo, update(_))
		.Times(1);
	EXPECT_CALL(mockSampleRepo, update(_))
		.WillOnce(SaveArg<0>(&capturedSample));

	ctrl->releaseOrder(TEST_ORDER_ID);

	EXPECT_EQ(capturedSample.getStock(), 150);
}

// 테스트 3: CONFIRMED 아닌 주문(PRODUCING)을 출고하면 logic_error 발생
TEST_F(ReleaseControllerTest, ReleaseOrder_NonConfirmedOrder_ThrowsLogicError)
{
	Order producingOrder(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", 150,
						 OrderStatus::PRODUCING, 0LL);

	EXPECT_CALL(mockOrderRepo, findById(TEST_ORDER_ID))
		.WillOnce(Return(producingOrder));
	EXPECT_CALL(mockSampleRepo, findById(_))
		.Times(0);
	EXPECT_CALL(mockOrderRepo, update(_))
		.Times(0);

	EXPECT_THROW(ctrl->releaseOrder(TEST_ORDER_ID), std::logic_error);
}

// 테스트 4: getConfirmedOrders는 CONFIRMED 상태 주문 목록을 반환한다
TEST_F(ReleaseControllerTest, GetConfirmedOrders_TwoConfirmedOrders_ReturnsBoth)
{
	std::vector<Order> confirmed = {
		Order(TEST_ORDER_ID,           TEST_SAMPLE_ID, "삼성전자",  100, OrderStatus::CONFIRMED, 0LL),
		Order("ORD-20260612-0002",     TEST_SAMPLE_ID, "SK하이닉스", 200, OrderStatus::CONFIRMED, 0LL)
	};

	EXPECT_CALL(mockOrderRepo, findByStatus(OrderStatus::CONFIRMED))
		.WillOnce(Return(confirmed));

	std::vector<Order> result = ctrl->getConfirmedOrders();

	EXPECT_EQ(result.size(), 2u);
}

// 테스트 5: 재고가 주문 수량보다 적으면 runtime_error 발생하고 update는 호출되지 않는다
TEST_F(ReleaseControllerTest, ReleaseOrder_InsufficientStock_ThrowsRuntimeErrorAndNoUpdate)
{
	EXPECT_CALL(mockOrderRepo, findById(TEST_ORDER_ID))
		.WillOnce(Return(makeConfirmedOrder(150)));
	EXPECT_CALL(mockSampleRepo, findById(TEST_SAMPLE_ID))
		.WillOnce(Return(makeSample(50)));
	EXPECT_CALL(mockOrderRepo, update(_))
		.Times(0);
	EXPECT_CALL(mockSampleRepo, update(_))
		.Times(0);

	EXPECT_THROW(ctrl->releaseOrder(TEST_ORDER_ID), std::runtime_error);
}