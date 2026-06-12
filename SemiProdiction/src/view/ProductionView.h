#pragma once
#include <vector>
#include <string>
#include "../model/ProductionJob.h"

class ProductionView {
public:
	void showProductionQueue(const std::vector<ProductionJob>& jobs) const;
	void showCurrentJob(const ProductionJob& job) const;
	void showQueueEmpty() const;
};
