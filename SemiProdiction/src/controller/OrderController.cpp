#include "OrderController.h"
#include <stdexcept>

OrderController::OrderController(IOrderRepository& orderRepo,
								 ISampleRepository& sampleRepo,
								 std::queue<ProductionJob>& prodQueue)
	: m_orderRepo(orderRepo)
	, m_sampleRepo(sampleRepo)
	, m_prodQueue(prodQueue)
{
}

std::string OrderController::reserveOrder(const std::string& sampleId,
										  const std::string& customerName,
										  int quantity)
{
	if (!m_sampleRepo.exists(sampleId))
		throw std::runtime_error("존재하지 않는 시료 ID: " + sampleId);

	int seq = m_orderRepo.getNextSequence();
	std::string orderId = Order::generateOrderId(seq);

	Order order(orderId, sampleId, customerName, quantity);
	m_orderRepo.save(order);

	return orderId;
}

std::vector<Order> OrderController::getReservedOrders() const
{
	return m_orderRepo.findByStatus(OrderStatus::RESERVED);
}

Order OrderController::getOrder(const std::string& orderId) const
{
	return m_orderRepo.findById(orderId);
}

std::vector<Order> OrderController::getAllOrders() const
{
	return m_orderRepo.findAll();
}

void OrderController::approveOrder(const std::string& orderId)
{
	throw std::logic_error("approveOrder: 미구현 (Phase 5)");
}

void OrderController::rejectOrder(const std::string& orderId)
{
	throw std::logic_error("rejectOrder: 미구현 (Phase 5)");
}