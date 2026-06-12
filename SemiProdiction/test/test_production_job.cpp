#include <gtest/gtest.h>
#include <cmath>
#include "src/model/ProductionJob.h"

// ============================================================
//  ProductionJobTest
// ============================================================

// 1. 실 생산량 공식 검증: ceil(170 / (0.92 * 0.9)) = ceil(205.31) = 206
TEST(ProductionJobTest, ActualProd_Formula_Case1)
{
	ProductionJob job("ORD-001", "S-001", 170, 0.92, 0.8);
	EXPECT_EQ(job.getActualProd(), 206);
}

// 2. 실 생산량 공식 검증: ceil(150 / (0.88 * 0.9)) = ceil(189.39) = 190
TEST(ProductionJobTest, ActualProd_Formula_Case2)
{
	ProductionJob job("ORD-002", "S-002", 150, 0.88, 0.5);
	EXPECT_EQ(job.getActualProd(), 190);
}

// 3. 총 생산 시간 = avgProdTime * actualProd
//    shortage=170, yield=0.92, avgProdTime=0.5 -> actualProd=206, totalTime=103.0
TEST(ProductionJobTest, TotalProdTime_IsAvgTimeMulActualProd)
{
	ProductionJob job("ORD-003", "S-001", 170, 0.92, 0.5);
	// actualProd = ceil(170/(0.92*0.9)) = 206, totalTime = 0.5 * 206 = 103.0
	EXPECT_DOUBLE_EQ(job.getTotalProdTime(), 0.5 * 206);
}

// 4. 생성 직후 isCompleted(startTime) == false
TEST(ProductionJobTest, IsCompleted_ReturnsFalseJustAfterStart)
{
	ProductionJob job("ORD-004", "S-001", 100, 0.92, 1.0);
	EXPECT_FALSE(job.isCompleted(job.getStartTime()));
}

// 5. completionTime > startTime 검증
TEST(ProductionJobTest, GetCompletionTime_IsGreaterThanStartTime)
{
	ProductionJob job("ORD-005", "S-001", 100, 0.92, 1.0);
	EXPECT_GT(job.getCompletionTime(), job.getStartTime());
}

// 6. 생성 직후 getCurrentProd(startTime) == 0
TEST(ProductionJobTest, GetCurrentProd_IsZeroJustAfterStart)
{
	ProductionJob job("ORD-006", "S-001", 100, 0.92, 1.0);
	EXPECT_EQ(job.getCurrentProd(job.getStartTime()), 0);
}
