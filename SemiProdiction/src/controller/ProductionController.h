#pragma once
#include <queue>
#include "../model/ProductionJob.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

class ProductionController {
public:
	explicit ProductionController(std::queue<ProductionJob>& prodQueue,
								  IOrderRepository& orderRepo,
								  ISampleRepository& sampleRepo);

	bool hasJob() const;
	ProductionJob peekNextJob() const;
	void completeCurrentJob();
	std::queue<ProductionJob> getQueue() const;

private:
	std::queue<ProductionJob>& m_prodQueue;
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
};
