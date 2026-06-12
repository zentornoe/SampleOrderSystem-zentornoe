#pragma once
#include <queue>
#include "../model/ProductionJob.h"

class ProductionView {
public:
	void showProductionQueue(const std::queue<ProductionJob>& queue) const;
	void showCurrentJob(const ProductionJob& job) const;
	void showCompleteSuccess(const std::string& orderId) const;
	void showQueueEmpty() const;
};
