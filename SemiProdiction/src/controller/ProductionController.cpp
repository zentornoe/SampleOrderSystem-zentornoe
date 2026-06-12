#include "ProductionController.h"
#include "../model/Order.h"
#include "../model/Sample.h"
#include <algorithm>
#include <stdexcept>

ProductionController::ProductionController(
	std::queue<ProductionJob>& prodQueue,
	IOrderRepository& orderRepo,
	ISampleRepository& sampleRepo)
	: m_prodQueue(prodQueue)
	, m_orderRepo(orderRepo)
	, m_sampleRepo(sampleRepo)
{}

void ProductionController::tick(long long nowSec) {
	if (m_prodQueue.empty()) return;

	const ProductionJob& front = m_prodQueue.front();
	if (!front.isCompleted(nowSec)) return;

	// 생산 완료: 주문 상태를 CONFIRMED 로 전이
	Order order = m_orderRepo.findById(front.getOrderId());
	order.completeProduction();
	m_orderRepo.update(order);

	// 생산된 수량을 재고에 반영
	Sample sample = m_sampleRepo.findById(front.getSampleId());
	sample.addStock(front.getActualProd());
	m_sampleRepo.update(sample);

	m_prodQueue.pop();
}

double ProductionController::getProgress(long long nowSec) const {
	if (m_prodQueue.empty()) return 0.0;

	const ProductionJob& front = m_prodQueue.front();
	if (front.getActualProd() == 0) return 0.0;

	double ratio = front.getCurrentProd(nowSec) / static_cast<double>(front.getActualProd());
	return std::min(100.0, ratio * 100.0);
}

bool ProductionController::hasJob() const {
	return !m_prodQueue.empty();
}

ProductionJob ProductionController::peekNextJob() const {
	if (m_prodQueue.empty())
		throw std::runtime_error("생산 큐가 비어 있습니다.");
	return m_prodQueue.front();
}

void ProductionController::completeCurrentJob() {
	if (!m_prodQueue.empty())
		m_prodQueue.pop();
}

std::queue<ProductionJob> ProductionController::getQueue() const {
	return m_prodQueue;
}
