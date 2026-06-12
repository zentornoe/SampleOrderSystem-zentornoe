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

	void   tick(long long nowSec);              // 완료 감지 및 상태 전이
	double getProgress(long long nowSec) const; // 현재 작업 진행률 0~100
	bool hasJob() const;
	ProductionJob peekNextJob() const;
	void completeCurrentJob();
	std::queue<ProductionJob> getQueue() const;

private:
	std::queue<ProductionJob>& m_prodQueue;
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
};
