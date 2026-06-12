#pragma once
#include <queue>
#include <vector>
#include "../model/ProductionJob.h"

class IOrderRepository;
class ISampleRepository;

class ProductionController {
public:
	explicit ProductionController(IOrderRepository& orderRepo,
								  ISampleRepository& sampleRepo,
								  std::queue<ProductionJob>& prodQueue);

	void   tick(long long nowSec);
	double getProgress(long long nowSec) const;
	bool hasJob() const;
	ProductionJob peekNextJob() const;
	std::vector<ProductionJob> getQueue() const;

private:
	std::queue<ProductionJob>& m_prodQueue;
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
};
