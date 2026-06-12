#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include "src/repository/JsonSampleRepository.h"

class JsonSampleRepositoryTest : public ::testing::Test {
protected:
	const std::string TEST_FILE = "temp_test_samples.json";

	void SetUp() override {
		std::ofstream f(TEST_FILE);
		f << "[]";
	}

	void TearDown() override {
		std::remove(TEST_FILE.c_str());
	}
};

// 1. save 후 findById로 모든 필드가 올바르게 복원되는지 검증
TEST_F(JsonSampleRepositoryTest, Save_And_FindById_RestoresAllFields)
{
	JsonSampleRepository repo(TEST_FILE);
	Sample s("S-001", "실리콘 웨이퍼", 0.5, 0.92, 480);
	repo.save(s);

	auto found = repo.findById("S-001");
	EXPECT_EQ(found.getId(), "S-001");
	EXPECT_EQ(found.getName(), "실리콘 웨이퍼");
	EXPECT_DOUBLE_EQ(found.getAvgProdTime(), 0.5);
	EXPECT_DOUBLE_EQ(found.getYield(), 0.92);
	EXPECT_EQ(found.getStock(), 480);
	EXPECT_EQ(found.getReservedQty(), 0);
}

// 2. update 후 변경된 재고가 파일에 반영되는지 검증
TEST_F(JsonSampleRepositoryTest, Update_PersistsStockChange)
{
	JsonSampleRepository repo(TEST_FILE);
	Sample s("S-002", "갈륨비소", 1.0, 0.85, 200);
	repo.save(s);

	s.addStock(100);
	repo.update(s);

	auto loaded = repo.findById("S-002");
	EXPECT_EQ(loaded.getStock(), 300);
}

// 3. findAll이 저장된 모든 시료를 순서대로 반환하는지 검증
TEST_F(JsonSampleRepositoryTest, FindAll_ReturnsAllSamplesInOrder)
{
	JsonSampleRepository repo(TEST_FILE);
	repo.save(Sample("S-003", "A형 웨이퍼", 1.0, 0.9, 100));
	repo.save(Sample("S-004", "B형 웨이퍼", 2.0, 0.8, 200));

	auto all = repo.findAll();
	ASSERT_EQ(all.size(), 2u);
	EXPECT_EQ(all[0].getId(), "S-003");
	EXPECT_EQ(all[1].getId(), "S-004");
}

// 4. exists가 존재/비존재를 정확히 구분하는지 검증
TEST_F(JsonSampleRepositoryTest, Exists_ReturnsCorrectly)
{
	JsonSampleRepository repo(TEST_FILE);
	repo.save(Sample("S-005", "테스트", 1.0, 0.9, 50));

	EXPECT_TRUE(repo.exists("S-005"));
	EXPECT_FALSE(repo.exists("S-999"));
}

// 5. findByName이 부분 일치 키워드로 검색되는지 검증
TEST_F(JsonSampleRepositoryTest, FindByName_ReturnsPartialMatches)
{
	JsonSampleRepository repo(TEST_FILE);
	repo.save(Sample("S-006", "실리콘 웨이퍼 A", 1.0, 0.9, 100));
	repo.save(Sample("S-007", "갈륨비소", 1.0, 0.8, 100));
	repo.save(Sample("S-008", "실리콘 웨이퍼 B", 1.0, 0.9, 100));

	auto result = repo.findByName("실리콘");
	ASSERT_EQ(result.size(), 2u);
	EXPECT_EQ(result[0].getId(), "S-006");
	EXPECT_EQ(result[1].getId(), "S-008");
}

// 6. reservedQty가 JSON에 올바르게 저장·복원되는지 검증
TEST_F(JsonSampleRepositoryTest, ReservedQty_PersistedAndRestored)
{
	JsonSampleRepository repo(TEST_FILE);
	Sample s("S-009", "테스트 시료", 1.0, 0.9, 300, 150);
	repo.save(s);

	auto loaded = repo.findById("S-009");
	EXPECT_EQ(loaded.getReservedQty(), 150);
	EXPECT_EQ(loaded.getAvailableStock(), 150);	// 300 - 150
}
