#include <gtest/gtest.h>
#include <stdexcept>
#include "src/model/Order.h"
#include "src/model/OrderStatus.h"

// ============================================================
//  OrderTest
// ============================================================

// 1. 기본 생성자 호출 시 상태가 RESERVED인지 검증
TEST(OrderTest, Constructor_DefaultStatusIsReserved)
{
	Order o("ORD-20260612-0001", "S-001", "삼성전자", 200);
	EXPECT_EQ(o.getStatus(), OrderStatus::RESERVED);
}

// 2. setStatus 호출 후 상태가 변경되는지 검증
TEST(OrderTest, SetStatus_ChangesStatus)
{
	Order o("ORD-20260612-0001", "S-001", "삼성전자", 200);
	o.setStatus(OrderStatus::CONFIRMED);
	EXPECT_EQ(o.getStatus(), OrderStatus::CONFIRMED);
}

// 3. REJECTED 상태에서 isMonitored가 false를 반환하는지 검증
TEST(OrderTest, IsMonitored_ReturnsFalseForRejected)
{
	Order o("ORD-20260612-0001", "S-001", "삼성전자", 200);
	o.setStatus(OrderStatus::REJECTED);
	EXPECT_FALSE(o.isMonitored());
}

// 4. 활성 상태(RESERVED, CONFIRMED, PRODUCING, RELEASE)에서 isMonitored가 true를 반환하는지 검증
TEST(OrderTest, IsMonitored_ReturnsTrueForActiveStatuses)
{
	Order o("ORD-20260612-0002", "S-001", "SK하이닉스", 100);

	o.setStatus(OrderStatus::RESERVED);
	EXPECT_TRUE(o.isMonitored());

	o.setStatus(OrderStatus::CONFIRMED);
	EXPECT_TRUE(o.isMonitored());

	o.setStatus(OrderStatus::PRODUCING);
	EXPECT_TRUE(o.isMonitored());

	o.setStatus(OrderStatus::RELEASE);
	EXPECT_TRUE(o.isMonitored());
}

// 5. generateOrderId가 올바른 형식을 반환하는지 검증
TEST(OrderTest, GenerateOrderId_FormatsCorrectly)
{
	std::string id = Order::generateOrderId(1);
	// 접두사 검증
	EXPECT_EQ(id.substr(0, 4), "ORD-");
	// 최소 길이 검증 (ORD-YYYYMMDD-NNNN = 17자)
	EXPECT_GE(id.length(), static_cast<std::size_t>(17));
	// 마지막 문자가 숫자인지 검증
	EXPECT_TRUE(std::isdigit(static_cast<unsigned char>(id.back())));
}

// 6. 수량이 0 이하일 때 std::invalid_argument가 발생하는지 검증
TEST(OrderTest, Constructor_NegativeQuantity_Throws)
{
	EXPECT_THROW(
		Order("ORD-20260612-0003", "S-001", "LG전자", 0),
		std::invalid_argument
	);
}
