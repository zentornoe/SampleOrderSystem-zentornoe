#include "MonitoringController.h"
#include "ProductionUtils.h"
#include "../model/OrderStatus.h"

MonitoringController::MonitoringController(IOrderRepository& orderRepo,
										   ISampleRepository& sampleRepo,
										   const std::queue<ProductionJob>& prodQueue)
	: m_orderRepo(orderRepo)
	, m_sampleRepo(sampleRepo)
	, m_prodQueue(prodQueue)
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

StockStatus MonitoringController::getStockStatus(const std::string& sampleId) const
{
	Sample sample = m_sampleRepo.findById(sampleId);
	int stock       = sample.getStock();
	int reservedQty = sample.getReservedQty();
	// 고갈 판단을 부족보다 먼저 해야 stock==0 && reservedQty>0 오분류를 막는다
	if (stock == 0)          return StockStatus::DEPLETED;
	if (stock < reservedQty) return StockStatus::SHORTAGE;
	return StockStatus::SUFFICIENT;
}

int MonitoringController::getEffectiveStock(const std::string& sampleId, long long nowSec) const
{
	Sample sample = m_sampleRepo.findById(sampleId);
	return sample.getStock() + getInProgressUnits(m_prodQueue, nowSec);
}

ProductionProgress MonitoringController::getProductionProgress(long long nowSec) const
{
	if (m_prodQueue.empty()) return {};
	const ProductionJob& job = m_prodQueue.front();
	ProductionProgress p;
	p.hasJob           = true;
	p.orderId          = job.getOrderId();
	p.sampleId         = job.getSampleId();
	p.currentProd      = job.getCurrentProd(nowSec);
	p.totalProd        = job.getActualProd();
	p.completionTimeSec = job.getCompletionTime();
	return p;
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