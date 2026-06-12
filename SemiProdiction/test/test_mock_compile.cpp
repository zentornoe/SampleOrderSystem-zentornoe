#include <gtest/gtest.h>
#include "test/mock/MockSampleRepository.h"
#include "test/mock/MockOrderRepository.h"

// Mock 클래스가 새로 추가된 메서드(findByName, findByStatus)를 포함하여
// 컴파일·링크되는지 확인하는 smoke test
TEST(MockCompileTest, MocksCompileWithAllInterfaceMethods)
{
	MockSampleRepository sampleRepo;
	MockOrderRepository orderRepo;
	EXPECT_TRUE(true);
}
