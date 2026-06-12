#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include <queue>
#include "src/controller/OrderController.h"
#include "test/mock/MockOrderRepository.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::SaveArg;
using ::testing::_;

// 1. 정상 예약 시 RESERVED 상태로 save가 1회 호출되는지 검증
TEST(OrderControllerTest, ReserveOrder_ValidInput_SaveCalledOnceWithReservedStatus)
{
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	OrderController ctrl(mockOrderRepo, mockSampleRepo, prodQueue);

	Order capturedOrder("ORD-00000000-0001", "S-001", "삼성전자", 200);

	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
	EXPECT_CALL(mockOrderRepo, save(_))
		.Times(1)
		.WillOnce(SaveArg<0>(&capturedOrder));

	ctrl.reserveOrder("S-001", "삼성전자", 200);

	EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::RESERVED);
}

// 2. 반환된 주문번호 형식이 ORD-YYYYMMDD-NNNN인지 검증
TEST(OrderControllerTest, ReserveOrder_ValidInput_ReturnsCorrectOrderIdFormat)
{
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	OrderController ctrl(mockOrderRepo, mockSampleRepo, prodQueue);

	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
	EXPECT_CALL(mockOrderRepo, save(_)).Times(1);

	std::string orderId = ctrl.reserveOrder("S-001", "삼성전자", 200);

	EXPECT_EQ(orderId.length(), 17u);
	EXPECT_EQ(orderId.substr(0, 4), "ORD-");
	EXPECT_EQ(orderId.substr(13, 4), "0001");
}

// 3. 존재하지 않는 시료 ID → runtime_error, getNextSequence·save 호출 0회
TEST(OrderControllerTest, ReserveOrder_NonExistentSampleId_ThrowsRuntimeError)
{
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	OrderController ctrl(mockOrderRepo, mockSampleRepo, prodQueue);

	EXPECT_CALL(mockSampleRepo, exists("X-999")).WillOnce(Return(false));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).Times(0);
	EXPECT_CALL(mockOrderRepo, save(_)).Times(0);

	EXPECT_THROW(
		ctrl.reserveOrder("X-999", "삼성전자", 200),
		std::runtime_error
	);
}

// 4. 수량 0 이하 → invalid_argument, save 0회 (경계값: 0, -1 모두 검증)
TEST(OrderControllerTest, ReserveOrder_ZeroOrNegativeQuantity_ThrowsInvalidArgument)
{
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	OrderController ctrl(mockOrderRepo, mockSampleRepo, prodQueue);

	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillRepeatedly(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).WillRepeatedly(Return(1));
	EXPECT_CALL(mockOrderRepo, save(_)).Times(0);

	EXPECT_THROW(
		ctrl.reserveOrder("S-001", "삼성전자", 0),
		std::invalid_argument
	);
	EXPECT_THROW(
		ctrl.reserveOrder("S-001", "삼성전자", -1),
		std::invalid_argument
	);
}

// 5. 빈 고객명 → invalid_argument, save 0회
TEST(OrderControllerTest, ReserveOrder_EmptyCustomerName_ThrowsInvalidArgument)
{
	MockOrderRepository  mockOrderRepo;
	MockSampleRepository mockSampleRepo;
	std::queue<ProductionJob> prodQueue;
	OrderController ctrl(mockOrderRepo, mockSampleRepo, prodQueue);

	EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
	EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
	EXPECT_CALL(mockOrderRepo, save(_)).Times(0);

	EXPECT_THROW(
		ctrl.reserveOrder("S-001", "", 200),
		std::invalid_argument
	);
}
