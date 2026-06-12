#pragma once
#include <queue>
#include <vector>
#include "../model/ProductionJob.h"

class IOrderRepository;
class ISampleRepository;

class ProductionController {
public:
	explicit ProductionController(std::queue<ProductionJob>& prodQueue,
								  IOrderRepository& orderRepo,
								  ISampleRepository& sampleRepo);

	void   tick(long long nowSec);              // 완료 감지 및 상태 전이
	double getProgress(long long nowSec) const; // 현재 작업 진행률 0~100
	bool hasJob() const;
	ProductionJob peekNextJob() const;
	std::vector<ProductionJob> getQueue() const;

private:
	std::queue<ProductionJob>& m_prodQueue;
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
};
