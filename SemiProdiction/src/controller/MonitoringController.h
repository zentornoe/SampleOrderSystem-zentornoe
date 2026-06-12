#pragma once
#include <vector>
#include <string>
#include <queue>
#include "../model/Order.h"
#include "../model/Sample.h"
#include "../model/MonitoringSummary.h"
#include "../model/ProductionJob.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

class MonitoringController {
public:
	explicit MonitoringController(IOrderRepository& orderRepo,
								  ISampleRepository& sampleRepo,
								  const std::queue<ProductionJob>& prodQueue);

	OrderSummary getOrderSummary() const;
	StockStatus  getStockStatus(const std::string& sampleId, int reservedQty) const;
	int          getEffectiveStock(const std::string& sampleId, long long nowSec) const;
	std::vector<Order> getActiveOrders() const;

private:
	IOrderRepository&               m_orderRepo;
	ISampleRepository&              m_sampleRepo;
	const std::queue<ProductionJob>& m_prodQueue;
};
