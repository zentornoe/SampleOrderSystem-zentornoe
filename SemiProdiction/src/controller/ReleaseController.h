#pragma once
#include <string>
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

class ReleaseController {
public:
	explicit ReleaseController(IOrderRepository& orderRepo,
							   ISampleRepository& sampleRepo);

	void releaseOrder(const std::string& orderId);

private:
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
};
