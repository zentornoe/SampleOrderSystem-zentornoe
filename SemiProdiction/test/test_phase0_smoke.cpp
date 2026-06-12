#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(Phase0SmokeTest, GoogleTestFrameworkWorks) {
	EXPECT_EQ(1 + 1, 2);
	EXPECT_TRUE(true);
	EXPECT_FALSE(false);
}

TEST(Phase0SmokeTest, GoogleMockFrameworkWorks) {
	using ::testing::Return;

	struct IStub {
		virtual ~IStub() = default;
		virtual int getValue() = 0;
	};
	struct MockStub : public IStub {
		MOCK_METHOD(int, getValue, (), (override));
	};

	MockStub mock;
	EXPECT_CALL(mock, getValue()).WillOnce(Return(42));
	EXPECT_EQ(mock.getValue(), 42);
}
