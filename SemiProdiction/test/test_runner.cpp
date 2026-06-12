#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Debug 빌드 진입점: 모든 테스트를 실행한다
int main(int argc, char** argv) {
	::testing::InitGoogleMock(&argc, argv);
	return RUN_ALL_TESTS();
}
