#include "MonitoringController.h"
#include "../model/OrderStatus.h"

MonitoringController::MonitoringController(IOrderRepository& orderRepo,
										   ISampleRepository& sampleRepo)
	: m_orderRepo(orderRepo)
	, m_sampleRepo(sampleRepo)
{
}

OrderSummary MonitoringController::getOrderSummary() const
{
	auto orders = m_orderRepo.findAll();
	OrderSummary summary;
	for (const auto& order : orders) {
		switch (order.getStatus()) {
		case OrderStatus::RESERVED:  summary.totalReserved++;  break;
		case OrderStatus::CONFIRMED: summary.totalConfirmed++; break;
		case OrderStatus::PRODUCING: summary.totalProducing++; break;
		case OrderStatus::RELEASE:   summary.totalRelease++;   break;
		default: break;
		}
	}
	return summary;
}

StockStatus MonitoringController::getStockStatus(const std::string& sampleId,
												  int reservedQty) const
{
	Sample sample = m_sampleRepo.findById(sampleId);
	int stock = sample.getStock();
	// 고갈 판단을 부족보다 먼저 해야 stock==0 && reservedQty>0 오분류를 막는다
	if (stock == 0)          return StockStatus::DEPLETED;
	if (stock < reservedQty) return StockStatus::SHORTAGE;
	return StockStatus::SUFFICIENT;
}

std::vector<Order> MonitoringController::getActiveOrders() const
{
	auto orders = m_orderRepo.findAll();
	std::vector<Order> result;
	for (const auto& order : orders) {
		if (order.isMonitored())
			result.push_back(order);
	}
	return result;
}