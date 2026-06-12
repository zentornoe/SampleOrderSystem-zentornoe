#pragma once
#include <vector>
#include <string>
#include "../model/ProductionJob.h"

class ProductionView {
public:
	void showProductionQueue(const std::vector<ProductionJob>& jobs, long long nowSec) const;
	void showCurrentJob(const ProductionJob& job, long long nowSec) const;
	void showQueueEmpty() const;

private:
	std::string formatTime(long long unixSec) const;
};
