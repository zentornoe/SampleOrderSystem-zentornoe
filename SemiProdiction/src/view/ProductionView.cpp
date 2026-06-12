#include "ProductionView.h"
#include <iostream>
#include <iomanip>

namespace {
	constexpr int TABLE_WIDTH = 60;
}

void ProductionView::showProductionQueue(const std::vector<ProductionJob>& jobs) const {
	if (jobs.empty()) { showQueueEmpty(); return; }
	std::cout << "\n[생산 대기 현황]\n";
	std::cout << std::left
	          << std::setw(20) << "주문 ID"
	          << std::setw(10) << "시료 ID"
	          << std::setw(12) << "실생산량"
	          << "완료 예정(초)\n";
	std::cout << std::string(TABLE_WIDTH, '-') << "\n";
	for (const auto& j : jobs) {
		std::cout << std::setw(20) << j.getOrderId()
		          << std::setw(10) << j.getSampleId()
		          << std::setw(12) << j.getActualProd()
		          << j.getCompletionTime() << "\n";
	}
}

void ProductionView::showCurrentJob(const ProductionJob& job) const {
	std::cout << "\n[현재 생산 중] " << job.getOrderId()
	          << " | 시료: " << job.getSampleId()
	          << " | 실생산량: " << job.getActualProd() << "\n";
}

void ProductionView::showQueueEmpty() const {
	std::cout << "현재 생산 대기 중인 작업이 없습니다.\n";
}
