#pragma once
#include <vector>
#include <string>
#include "../model/Order.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

class ReleaseController {
public:
	explicit ReleaseController(IOrderRepository& orderRepo,
	                           ISampleRepository& sampleRepo);

	std::vector<Order> getConfirmedOrders() const;
	void releaseOrder(const std::string& orderId);

private:
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
};
