#pragma once
#include <vector>
#include <string>
#include "../model/Order.h"
#include "../model/Sample.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

enum class StockStatus {
	SUFFICIENT,
	SHORTAGE,
	DEPLETED
};

struct OrderSummary {
	int totalReserved = 0;
	int totalConfirmed = 0;
	int totalProducing = 0;
	int totalRelease = 0;
};

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
