#include "ProductionView.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdio>

namespace {
	constexpr int TABLE_WIDTH = 72;
}

std::string ProductionView::formatTime(long long unixSec) const
{
	std::time_t t = static_cast<std::time_t>(unixSec);
	std::tm tm_buf{};
#ifdef _WIN32
	localtime_s(&tm_buf, &t);
#else
	localtime_r(&t, &tm_buf);
#endif
	char buf[20];
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
	return buf;
}

void ProductionView::showProductionQueue(const std::vector<ProductionJob>& jobs, long long nowSec) const
{
	if (jobs.empty()) { showQueueEmpty(); return; }
	std::cout << "\n[생산 대기 현황]\n";
	std::cout << std::left
	          << std::setw(22) << "주문 ID"
	          << std::setw(10) << "시료 ID"
	          << std::setw(18) << "진행 (개/%)"
	          << "완료 예정\n";
	std::cout << std::string(TABLE_WIDTH, '-') << "\n";
	for (size_t i = 0; i < jobs.size(); ++i) {
		const auto& j   = jobs[i];
		int total       = j.getActualProd();
		// 대기 중(i>0) 작업은 아직 시작 안 했으므로 진행 0으로 표시
		int current     = (i == 0) ? j.getCurrentProd(nowSec) : 0;
		int pct         = (total > 0) ? static_cast<int>(current * 100.0 / total) : 0;
		std::string progress = std::to_string(current) + "/" + std::to_string(total)
		                     + "(" + std::to_string(pct) + "%)";
		std::cout << std::setw(22) << j.getOrderId()
		          << std::setw(10) << j.getSampleId()
		          << std::setw(18) << progress
		          << formatTime(j.getCompletionTime()) << "\n";
	}
}

void ProductionView::showCurrentJob(const ProductionJob& job, long long nowSec) const
{
	int total   = job.getActualProd();
	int current = job.getCurrentProd(nowSec);
	int pct     = (total > 0) ? static_cast<int>(current * 100.0 / total) : 0;
	std::cout << "\n[현재 생산 중] " << job.getOrderId()
	          << " | 시료: "   << job.getSampleId()
	          << " | 진행: "   << current << "/" << total << "개 (" << pct << "%)"
	          << " | 완료 예정: " << formatTime(job.getCompletionTime()) << "\n";
}

void ProductionView::showQueueEmpty() const
{
	std::cout << "현재 생산 대기 중인 작업이 없습니다.\n";
}