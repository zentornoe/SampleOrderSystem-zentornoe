#pragma once
#include <vector>
#include <string>
#include "../model/Order.h"
#include "../model/Sample.h"
#include "../model/MonitoringSummary.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

class MonitoringController {
public:
	explicit MonitoringController(IOrderRepository& orderRepo,
								  ISampleRepository& sampleRepo);

	OrderSummary getOrderSummary() const;
	StockStatus getStockStatus(const std::string& sampleId, int reservedQty) const;
	std::vector<Order> getActiveOrders() const;

private:
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
};
