#include "ReleaseController.h"
#include "../model/OrderStatus.h"
#include <stdexcept>

ReleaseController::ReleaseController(IOrderRepository& orderRepo,
                                     ISampleRepository& sampleRepo)
	: m_orderRepo(orderRepo)
	, m_sampleRepo(sampleRepo)
{
}

std::vector<Order> ReleaseController::getConfirmedOrders() const
{
	return m_orderRepo.findByStatus(OrderStatus::CONFIRMED);
}

void ReleaseController::releaseOrder(const std::string& orderId)
{
	Order order = m_orderRepo.findById(orderId);

	if (order.getStatus() != OrderStatus::CONFIRMED)
		throw std::logic_error("출고 가능한 주문이 아닙니다: " + orderId);

	Sample sample = m_sampleRepo.findById(order.getSampleId());

	if (sample.getStock() < order.getQuantity())
		throw std::runtime_error("재고 부족으로 출고할 수 없습니다: " + orderId);

	order.release();
	m_orderRepo.update(order);

	sample.reduceStock(order.getQuantity());
	// 직접 CONFIRMED 경로에서만 reservedQty가 설정되므로 조건부 해제
	if (sample.getReservedQty() >= order.getQuantity())
		sample.releaseQty(order.getQuantity());
	m_sampleRepo.update(sample);
}