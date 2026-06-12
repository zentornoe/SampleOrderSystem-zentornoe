#include <gtest/gtest.h>
#include <stdexcept>
#include "src/model/Sample.h"
#include "src/model/OrderStatus.h"

// ============================================================
//  SampleTest
// ============================================================

// 1. 생성자가 모든 값을 올바르게 저장하는지 검증
TEST(SampleTest, Constructor_StoresAllValues)
{
	Sample s("S-001", "실리콘 웨이퍼", 0.5, 0.92, 480);
	EXPECT_EQ(s.getId(),          "S-001");
	EXPECT_EQ(s.getName(),        "실리콘 웨이퍼");
	EXPECT_DOUBLE_EQ(s.getAvgProdTime(), 0.5);
	EXPECT_DOUBLE_EQ(s.getYield(),       0.92);
	EXPECT_EQ(s.getStock(),       480);
}

// 2. addStock 호출 후 재고가 증가하는지 검증
TEST(SampleTest, AddStock_IncreasesStock)
{
	Sample s("S-002", "test", 1.0, 0.9, 100);
	s.addStock(50);
	EXPECT_EQ(s.getStock(), 150);
}

// 3. reduceStock 호출 후 재고가 감소하는지 검증
TEST(SampleTest, ReduceStock_DecreasesStock)
{
	Sample s("S-003", "test", 1.0, 0.9, 100);
	s.reduceStock(30);
	EXPECT_EQ(s.getStock(), 70);
}

// 4. 재고보다 많은 수량 차감 시 std::runtime_error가 발생하는지 검증
TEST(SampleTest, ReduceStock_WhenInsufficient_Throws)
{
	Sample s("S-004", "test", 1.0, 0.9, 10);
	EXPECT_THROW(s.reduceStock(20), std::runtime_error);
}

// 5. isStockEnough 경계값 검증: 정확히 같을 때 true, 초과 시 false
TEST(SampleTest, IsStockEnough_BoundaryValue)
{
	Sample s("S-005", "test", 1.0, 0.9, 100);
	EXPECT_TRUE(s.isStockEnough(100));
	EXPECT_FALSE(s.isStockEnough(101));
}

// ============================================================
//  OrderStatusTest
// ============================================================

// 6. toString/fromString 왕복(round-trip) 검증 + 잘못된 문자열 예외
TEST(OrderStatusTest, ToString_And_FromString_RoundTrip)
{
	// 5개 상태 왕복 검증
	EXPECT_EQ(fromString(toString(OrderStatus::RESERVED)),  OrderStatus::RESERVED);
	EXPECT_EQ(fromString(toString(OrderStatus::CONFIRMED)), OrderStatus::CONFIRMED);
	EXPECT_EQ(fromString(toString(OrderStatus::PRODUCING)), OrderStatus::PRODUCING);
	EXPECT_EQ(fromString(toString(OrderStatus::RELEASE)),   OrderStatus::RELEASE);
	EXPECT_EQ(fromString(toString(OrderStatus::REJECTED)),  OrderStatus::REJECTED);

	// 알 수 없는 문자열 -> std::invalid_argument
	EXPECT_THROW(fromString("UNKNOWN"), std::invalid_argument);
}
