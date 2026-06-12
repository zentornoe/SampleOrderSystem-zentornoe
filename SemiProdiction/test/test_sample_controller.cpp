#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <stdexcept>
#include "src/controller/SampleController.h"
#include "test/mock/MockSampleRepository.h"

using ::testing::Return;
using ::testing::_;

// ============================================================
//  SampleControllerTest
// ============================================================

// 1. 정상 등록 시 Repository의 save가 1회 호출되는지 검증
TEST(SampleControllerTest, RegisterSample_CallsSaveOnRepository)
{
	MockSampleRepository mockRepo;
	EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(false));
	EXPECT_CALL(mockRepo, save(_)).Times(1);

	SampleController ctrl(mockRepo);
	ctrl.registerSample("S-001", "실리콘 웨이퍼", 0.5, 0.92, 480);
}

// 2. 중복 ID로 등록 시 runtime_error가 발생하는지 검증
TEST(SampleControllerTest, RegisterSample_DuplicateId_Throws)
{
	MockSampleRepository mockRepo;
	EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(true));
	EXPECT_CALL(mockRepo, save(_)).Times(0);

	SampleController ctrl(mockRepo);
	EXPECT_THROW(
		ctrl.registerSample("S-001", "실리콘 웨이퍼", 0.5, 0.92, 480),
		std::runtime_error
	);
}

// 3. 수율이 범위(0 초과 ~ 1 이하)를 벗어나면 invalid_argument가 발생하는지 검증
TEST(SampleControllerTest, RegisterSample_InvalidYield_Throws)
{
	MockSampleRepository mockRepo;
	EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(false));

	SampleController ctrl(mockRepo);
	EXPECT_THROW(
		ctrl.registerSample("S-001", "test", 0.5, 1.5, 100),
		std::invalid_argument
	);
}

// 4. 평균 생산시간이 0 이하이면 invalid_argument가 발생하는지 검증
TEST(SampleControllerTest, RegisterSample_InvalidAvgProdTime_Throws)
{
	MockSampleRepository mockRepo;
	EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(false));

	SampleController ctrl(mockRepo);
	EXPECT_THROW(
		ctrl.registerSample("S-001", "test", -1.0, 0.9, 100),
		std::invalid_argument
	);
}

// 5. getAllSamples가 Repository 결과를 그대로 반환하는지 검증
TEST(SampleControllerTest, GetAllSamples_DelegatesToRepository)
{
	MockSampleRepository mockRepo;
	std::vector<Sample> samples = {
		Sample("S-001", "웨이퍼A", 0.5, 0.9, 100),
		Sample("S-002", "웨이퍼B", 1.0, 0.8, 200)
	};
	EXPECT_CALL(mockRepo, findAll()).WillOnce(Return(samples));

	SampleController ctrl(mockRepo);
	auto result = ctrl.getAllSamples();

	ASSERT_EQ(result.size(), 2u);
	EXPECT_EQ(result[0].getId(), "S-001");
	EXPECT_EQ(result[1].getId(), "S-002");
}

// 6. searchByName이 Repository에 키워드를 그대로 위임하는지 검증
TEST(SampleControllerTest, SearchByName_DelegatesToRepository)
{
	MockSampleRepository mockRepo;
	std::vector<Sample> matched = {
		Sample("S-001", "실리콘 웨이퍼 A", 0.5, 0.9, 100),
		Sample("S-003", "실리콘 웨이퍼 B", 0.5, 0.9, 150)
	};
	EXPECT_CALL(mockRepo, findByName("웨이퍼")).WillOnce(Return(matched));

	SampleController ctrl(mockRepo);
	auto result = ctrl.searchByName("웨이퍼");

	EXPECT_EQ(result.size(), 2u);
}

// 7. 검색 결과가 없을 때 빈 vector를 반환하는지 검증
TEST(SampleControllerTest, SearchByName_EmptyResult_ReturnsEmptyVector)
{
	MockSampleRepository mockRepo;
	EXPECT_CALL(mockRepo, findByName(_))
		.WillOnce(Return(std::vector<Sample>{}));

	SampleController ctrl(mockRepo);
	auto result = ctrl.searchByName("없는키워드");

	EXPECT_TRUE(result.empty());
}
