#include "ProductionController.h"
#include "../model/Order.h"
#include "../model/Sample.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"
#include <algorithm>
#include <stdexcept>

ProductionController::ProductionController(
	IOrderRepository& orderRepo,
	ISampleRepository& sampleRepo,
	std::queue<ProductionJob>& prodQueue)
	: m_orderRepo(orderRepo)
	, m_sampleRepo(sampleRepo)
	, m_prodQueue(prodQueue)
{}

void ProductionController::tick(long long nowSec) {
	if (m_prodQueue.empty()) return;

	const ProductionJob& front = m_prodQueue.front();
	if (!front.isCompleted(nowSec)) return;

	Order order = m_orderRepo.findById(front.getOrderId());
	order.completeProduction();
	m_orderRepo.update(order);

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

std::vector<ProductionJob> ProductionController::getQueue() const {
	std::queue<ProductionJob> copy = m_prodQueue;
	std::vector<ProductionJob> result;
	while (!copy.empty()) {
		result.push_back(copy.front());
		copy.pop();
	}
	return result;
}
