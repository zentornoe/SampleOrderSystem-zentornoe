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

	if (!sample.isStockEnough(order.getQuantity()))
		throw std::runtime_error("재고 부족으로 출고할 수 없습니다: " + orderId);

	// 재고를 먼저 차감해야 order 상태 변경 후 재고 저장 실패 시 불일치를 막는다
	sample.reduceStock(order.getQuantity());
	sample.releaseQty(order.getQuantity());
	m_sampleRepo.update(sample);

	order.release();
	m_orderRepo.update(order);
}