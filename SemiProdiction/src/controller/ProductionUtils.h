#pragma once
#include <queue>
#include "../model/ProductionJob.h"

// OrderController / MonitoringController 양쪽에서 공유되는 생산 진척 계산 헬퍼
inline int getInProgressUnits(const std::queue<ProductionJob>& q, long long nowSec)
{
	return q.empty() ? 0 : q.front().getCurrentProd(nowSec);
}