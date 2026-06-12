#include "OrderController.h"
#include <stdexcept>
#include <cmath>

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

	if (quantity <= 0)
		throw std::invalid_argument("수량은 1 이상이어야 합니다.");

	if (customerName.empty())
		throw std::invalid_argument("고객명은 비어있을 수 없습니다.");

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
	Order order = m_orderRepo.findById(orderId);

	if (order.getStatus() != OrderStatus::RESERVED)
		throw std::logic_error("승인 가능한 주문이 아닙니다: " + orderId);

	Sample sample = m_sampleRepo.findById(order.getSampleId());

	if (sample.isStockEnough(order.getQuantity()))
	{
		// 재고 충분 → 즉시 CONFIRMED
		order.confirm();
		sample.reserveQty(order.getQuantity());
		m_sampleRepo.update(sample);
		m_orderRepo.update(order);
	}
	else
	{
		// 재고 부족 → 생산라인 등록 후 PRODUCING
		int shortage = order.getQuantity() - sample.getStock();
		ProductionJob job(order.getOrderId(), order.getSampleId(),
						  shortage, sample.getYield(), sample.getAvgProdTime());
		m_prodQueue.push(job);
		order.sendToProduction();
		m_orderRepo.update(order);
	}
}

void OrderController::rejectOrder(const std::string& orderId)
{
	Order order = m_orderRepo.findById(orderId);
	order.reject();
	m_orderRepo.update(order);
}