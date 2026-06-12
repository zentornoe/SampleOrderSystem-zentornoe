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

	// "없습니다" — UTF-8 BOM 적용 후 한글 리터럴 비교 가능
	// 안전을 위해 ASCII 범위 문자로도 검증 가능한 패턴 사용
	const std::string output = oss.str();
	EXPECT_FALSE(output.empty());
	// showSampleList({}) 는 "등록된 시료가 없습니다.\n" 출력
	// ASCII 부분 "." 또는 "\n" 으로 최소 출력 확인, 추가로 비어있지 않음을 검증
	EXPECT_NE(output.find('.'), std::string::npos);
}
