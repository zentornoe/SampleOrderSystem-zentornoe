#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <queue>
#include <memory>
#include "src/controller/OrderController.h"
#include "test/mock/MockOrderRepository.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

namespace {
	const std::string TEST_SAMPLE_ID = "S-001";
	const std::string TEST_ORDER_ID  = "ORD-20260612-0001";
}

// ============================================================
//  공통 Fixture — Mock + Controller 셋업 중복 제거
// ============================================================
class OrderControllerTest : public ::testing::Test {
protected:
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	std::unique_ptr<OrderController> ctrl;

	void SetUp() override {
		ctrl = std::make_unique<OrderController>(mockOrderRepo, mockSampleRepo, prodQueue);
	}
};

// 1. 정상 예약 시 RESERVED 상태로 save가 1회 호출되는지 검증
TEST_F(OrderControllerTest, ReserveOrder_ValidInput_SaveCalledOnceWithReservedStatus)
{
	Order capturedOrder("ORD-00000000-0001", "S-001", "삼성전자", 200);

	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
	EXPECT_CALL(mockOrderRepo, save(_))
		.Times(1)
		.WillOnce(SaveArg<0>(&capturedOrder));

	ctrl->reserveOrder("S-001", "삼성전자", 200);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::RESERVED);
}

// 2. 반환된 주문번호 형식이 ORD-YYYYMMDD-NNNN인지 검증
TEST_F(OrderControllerTest, ReserveOrder_ValidInput_ReturnsCorrectOrderIdFormat)
{
	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
	EXPECT_CALL(mockOrderRepo, save(_)).Times(1);

	std::string orderId = ctrl->reserveOrder("S-001", "삼성전자", 200);

	EXPECT_EQ(orderId.length(), 17u);
	EXPECT_EQ(orderId.substr(0, 4), "ORD-");
	EXPECT_EQ(orderId.substr(13, 4), "0001");
}

// 3. 존재하지 않는 시료 ID → runtime_error, getNextSequence·save 호출 0회
TEST_F(OrderControllerTest, ReserveOrder_NonExistentSampleId_ThrowsRuntimeError)
{
	EXPECT_CALL(mockSampleRepo, exists("X-999")).WillOnce(Return(false));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).Times(0);
	EXPECT_CALL(mockOrderRepo, save(_)).Times(0);

	EXPECT_THROW(
		ctrl->reserveOrder("X-999", "삼성전자", 200),
		std::runtime_error
	);
}

// 4. 수량 0 이하 → invalid_argument, getNextSequence·save 호출 0회 (경계값: 0, -1)
TEST_F(OrderControllerTest, ReserveOrder_ZeroOrNegativeQuantity_ThrowsInvalidArgument)
{
	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillRepeatedly(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).Times(0);
	EXPECT_CALL(mockOrderRepo, save(_)).Times(0);

	EXPECT_THROW(
		ctrl->reserveOrder("S-001", "삼성전자", 0),
		std::invalid_argument
	);
	EXPECT_THROW(
		ctrl->reserveOrder("S-001", "삼성전자", -1),
		std::invalid_argument
	);
}

// 5. 빈 고객명 → invalid_argument, getNextSequence·save 호출 0회
TEST_F(OrderControllerTest, ReserveOrder_EmptyCustomerName_ThrowsInvalidArgument)
{
	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).Times(0);
	EXPECT_CALL(mockOrderRepo, save(_)).Times(0);

	EXPECT_THROW(
		ctrl->reserveOrder("S-001", "", 200),
		std::invalid_argument
	);
}

// 6. 재고 충분(200 >= 100) 시 CONFIRMED 전환, prodQueue 비어있음
TEST_F(OrderControllerTest, ApproveOrder_SufficientStock_OrderBecomesConfirmed)
{
	Order  inOrder(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", 100);
	Sample inSample(TEST_SAMPLE_ID, "테스트시료", 5.0, 0.92, 200);
	Order  capturedOrder = inOrder;

	EXPECT_CALL(mockOrderRepo,  findById(TEST_ORDER_ID)).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(TEST_SAMPLE_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
	EXPECT_CALL(mockSampleRepo, update(_)).Times(1);

	ctrl->approveOrder(TEST_ORDER_ID);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
	EXPECT_TRUE(prodQueue.empty());
}

// 7. 재고 부족(30 < 200) 시 PRODUCING 전환, prodQueue에 1건 등록, reservedQty 저장
TEST_F(OrderControllerTest, ApproveOrder_InsufficientStock_OrderBecomesProducing)
{
	Order  inOrder(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", 200);
	Sample inSample(TEST_SAMPLE_ID, "테스트시료", 5.0, 0.92, 30);
	Order  capturedOrder  = inOrder;
	Sample capturedSample = inSample;

	EXPECT_CALL(mockOrderRepo,  findById(TEST_ORDER_ID)).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(TEST_SAMPLE_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));

	ctrl->approveOrder(TEST_ORDER_ID);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::PRODUCING);
	EXPECT_EQ(prodQueue.size(), 1u);
	EXPECT_EQ(capturedSample.getReservedQty(), 200);
}

// 8. 재고 = 주문 수량 경계값(100 == 100) → CONFIRMED, prodQueue 비어있음
TEST_F(OrderControllerTest, ApproveOrder_StockEqualsQuantity_OrderBecomesConfirmed)
{
	Order  inOrder(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", 100);
	Sample inSample(TEST_SAMPLE_ID, "테스트시료", 5.0, 0.92, 100);
	Order  capturedOrder = inOrder;

	EXPECT_CALL(mockOrderRepo,  findById(TEST_ORDER_ID)).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(TEST_SAMPLE_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
	EXPECT_CALL(mockSampleRepo, update(_)).Times(1);

	ctrl->approveOrder(TEST_ORDER_ID);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
	EXPECT_TRUE(prodQueue.empty());
}

// 9. RESERVED 아닌 주문(CONFIRMED) 승인 시 logic_error, sampleRepo·update 호출 0회
TEST_F(OrderControllerTest, ApproveOrder_NotReservedOrder_ThrowsLogicError)
{
	Order inOrder(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", 100,
	              OrderStatus::CONFIRMED, 0LL);

	EXPECT_CALL(mockOrderRepo,  findById(TEST_ORDER_ID)).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(_)).Times(0);
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(0);

	EXPECT_THROW(ctrl->approveOrder(TEST_ORDER_ID), std::logic_error);
}

// 10. RESERVED 주문 거절 시 REJECTED 전환
TEST_F(OrderControllerTest, RejectOrder_ReservedOrder_OrderBecomesRejected)
{
	Order inOrder(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", 100);
	Order capturedOrder = inOrder;

	EXPECT_CALL(mockOrderRepo, findById(TEST_ORDER_ID)).WillOnce(Return(inOrder));
	EXPECT_CALL(mockOrderRepo, update(_)).WillOnce(SaveArg<0>(&capturedOrder));

	ctrl->rejectOrder(TEST_ORDER_ID);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::REJECTED);
}

// 11. RESERVED 목록 조회가 Repository에 위임되어 2건 반환
TEST_F(OrderControllerTest, GetReservedOrders_ReturnsTwoOrders)
{
	std::vector<Order> reserved = {
		Order(TEST_ORDER_ID,          TEST_SAMPLE_ID, "삼성전자",  100),
		Order("ORD-20260612-0002",    TEST_SAMPLE_ID, "SK하이닉스", 50)
	};

	EXPECT_CALL(mockOrderRepo, findByStatus(OrderStatus::RESERVED)).WillOnce(Return(reserved));

	auto result = ctrl->getReservedOrders();

	EXPECT_EQ(result.size(), 2u);
}

// 12. 재고 부족 승인 시 생산량 공식(206) 검증
//     reservedQty=0, stock=30 → available=30, shortage=200-30=170
//     actualProd = ceil(170 / (0.92 * 0.9)) = ceil(205.31) = 206
TEST_F(OrderControllerTest, ApproveOrder_InsufficientStock_ProductionJobHasCorrectActualProd)
{
	Order  inOrder(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", 200);
	Sample inSample(TEST_SAMPLE_ID, "테스트시료", 5.0, 0.92, 30);

	EXPECT_CALL(mockOrderRepo,  findById(TEST_ORDER_ID)).WillOnce(Return(inOrder));
	EXPECT_CALL(mockSampleRepo, findById(TEST_SAMPLE_ID)).WillOnce(Return(inSample));
	EXPECT_CALL(mockSampleRepo, update(_)).Times(1);
	EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);

	ctrl->approveOrder(TEST_ORDER_ID);

	ASSERT_FALSE(prodQueue.empty());
	EXPECT_EQ(prodQueue.front().getActualProd(), 206);
	EXPECT_EQ(prodQueue.front().getOrderId(), TEST_ORDER_ID);
}