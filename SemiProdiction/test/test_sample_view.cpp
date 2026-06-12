#include <gtest/gtest.h>
#include <sstream>
#include "src/view/SampleView.h"

// ============================================================
//  SampleViewTest
// ============================================================

// 8. showSampleList 출력에 ID, 이름, 재고가 포함되는지 검증
TEST(SampleViewTest, ShowSampleList_OutputContainsIdNameStock)
{
	SampleView view;
	std::vector<Sample> samples = {
		Sample("S-001", "TestSample", 0.5, 0.92, 480)
	};

	std::ostringstream oss;
	std::streambuf* oldBuf = std::cout.rdbuf(oss.rdbuf());
	view.showSampleList(samples);
	std::cout.rdbuf(oldBuf);

	const std::string output = oss.str();
	EXPECT_NE(output.find("S-001"),      std::string::npos);
	EXPECT_NE(output.find("TestSample"), std::string::npos);
	EXPECT_NE(output.find("480"),        std::string::npos);
}

// 9. 빈 목록일 때 안내 문구가 출력되는지 검증
TEST(SampleViewTest, ShowSampleList_EmptyList_ShowsNotice)
{
	SampleView view;

	std::ostringstream oss;
	std::streambuf* oldBuf = std::cout.rdbuf(oss.rdbuf());
	view.showSampleList({});
	std::cout.rdbuf(oldBuf);

	const std::string output = oss.str();
	EXPECT_FALSE(output.empty());
	// "없습니다.\n" — 문장 끝 마침표+개행: '.' 단독보다 구체적
	EXPECT_NE(output.find(".\n"), std::string::npos);
}
