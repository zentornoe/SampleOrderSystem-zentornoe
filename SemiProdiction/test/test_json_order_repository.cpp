#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include "src/repository/JsonOrderRepository.h"

class JsonOrderRepositoryTest : public ::testing::Test {
protected:
	const std::string TEST_FILE = "temp_test_orders.json";

	void SetUp() override {
		std::ofstream f(TEST_FILE);
		f << "[]";
	}

	void TearDown() override {
		std::remove(TEST_FILE.c_str());
	}
};

// 1. save 후 findById로 모든 필드가 올바르게 복원되는지 검증
TEST_F(JsonOrderRepositoryTest, Save_And_FindById_RestoresAllFields)
{
	JsonOrderRepository repo(TEST_FILE);
	Order o("ORD-20260612-0001", "S-001", "삼성전자", 100);
	repo.save(o);

	auto found = repo.findById("ORD-20260612-0001");
	EXPECT_EQ(found.getOrderId(), "ORD-20260612-0001");
	EXPECT_EQ(found.getSampleId(), "S-001");
	EXPECT_EQ(found.getCustomerName(), "삼성전자");
	EXPECT_EQ(found.getQuantity(), 100);
	EXPECT_EQ(found.getStatus(), OrderStatus::RESERVED);
}

// 2. update 후 상태 변경이 파일에 반영되는지 검증
TEST_F(JsonOrderRepositoryTest, Update_PersistsStatusChange)
{
	JsonOrderRepository repo(TEST_FILE);
	Order o("ORD-20260612-0002", "S-001", "SK하이닉스", 50);
	repo.save(o);

	o.confirm();
	repo.update(o);

	auto loaded = repo.findById("ORD-20260612-0002");
	EXPECT_EQ(loaded.getStatus(), OrderStatus::CONFIRMED);
}

// 3. findByStatus가 해당 상태의 주문만 반환하는지 검증
TEST_F(JsonOrderRepositoryTest, FindByStatus_ReturnsOnlyMatchingOrders)
{
	JsonOrderRepository repo(TEST_FILE);
	Order o1("ORD-20260612-0003", "S-001", "고객A", 10);
	Order o2("ORD-20260612-0004", "S-001", "고객B", 20);
	o2.confirm();
	repo.save(o1);
	repo.save(o2);

	auto reserved = repo.findByStatus(OrderStatus::RESERVED);
	ASSERT_EQ(reserved.size(), 1u);
	EXPECT_EQ(reserved[0].getOrderId(), "ORD-20260612-0003");

	auto confirmed = repo.findByStatus(OrderStatus::CONFIRMED);
	ASSERT_EQ(confirmed.size(), 1u);
	EXPECT_EQ(confirmed[0].getOrderId(), "ORD-20260612-0004");
}

// 4. getNextSequence가 저장 건수 + 1을 반환하는지 검증
TEST_F(JsonOrderRepositoryTest, GetNextSequence_ReturnsCountPlusOne)
{
	JsonOrderRepository repo(TEST_FILE);
	EXPECT_EQ(repo.getNextSequence(), 1);

	repo.save(Order("ORD-20260612-0005", "S-001", "고객C", 30));
	EXPECT_EQ(repo.getNextSequence(), 2);

	repo.save(Order("ORD-20260612-0006", "S-001", "고객D", 40));
	EXPECT_EQ(repo.getNextSequence(), 3);
}

// 5. 존재하지 않는 ID로 findById 호출 시 std::runtime_error가 발생하는지 검증
TEST_F(JsonOrderRepositoryTest, FindById_WhenNotFound_Throws)
{
	JsonOrderRepository repo(TEST_FILE);
	EXPECT_THROW(repo.findById("ORD-99999999-9999"), std::runtime_error);
}
