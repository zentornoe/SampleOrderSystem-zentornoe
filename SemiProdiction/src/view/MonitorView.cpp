#include "MonitorView.h"
#include "../model/OrderStatus.h"
#include <iostream>
#include <iomanip>
#include <ctime>
#include <cstdio>

namespace {
	constexpr int TABLE_WIDTH = 64;
}

void MonitorView::showOrderSummary(const OrderSummary& summary) const
{
	std::cout << "=== 주문 현황 ===\n"
		<< std::string(TABLE_WIDTH, '-') << "\n"
		<< "접수 대기 (RESERVED) : " << summary.totalReserved  << "건\n"
		<< "생산 중   (PRODUCING): " << summary.totalProducing << "건\n"
		<< "출고 대기 (CONFIRMED): " << summary.totalConfirmed << "건\n"
		<< "출고 완료 (RELEASE)  : " << summary.totalRelease   << "건\n";
}

void MonitorView::showActiveOrders(const std::vector<Order>& orders) const
{
	if (orders.empty()) {
		std::cout << "활성 주문이 없습니다.\n";
		return;
	}

	std::cout << "=== 활성 주문 목록 ===\n"
		<< std::string(TABLE_WIDTH, '-') << "\n"
		<< std::left
		<< std::setw(20) << "주문번호"
		<< std::setw(8)  << "시료"
		<< std::setw(12) << "고객명"
		<< std::setw(6)  << "수량"
		<< "상태\n"
		<< std::string(TABLE_WIDTH, '-') << "\n";
	for (const auto& order : orders) {
		std::cout << std::left
			<< std::setw(20) << order.getOrderId()
			<< std::setw(8)  << order.getSampleId()
			<< std::setw(12) << order.getCustomerName()
			<< std::setw(6)  << order.getQuantity()
			<< toString(order.getStatus())
			<< "\n";
	}
}

void MonitorView::showProductionProgress(const ProductionProgress& p) const
{
	std::cout << "=== 생산 진행 현황 ===\n"
	          << std::string(TABLE_WIDTH, '-') << "\n";
	if (!p.hasJob) {
		std::cout << "현재 생산 중인 작업이 없습니다.\n";
		return;
	}

	// 완료 예정 시각: Unix timestamp → YYYY-MM-DD HH:MM
	std::time_t t = static_cast<std::time_t>(p.completionTimeSec);
	std::tm tm_buf{};
#ifdef _WIN32
	localtime_s(&tm_buf, &t);
#else
	localtime_r(&t, &tm_buf);
#endif
	char timeBuf[20];
	std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M", &tm_buf);

	int pct = (p.totalProd > 0)
	          ? static_cast<int>(p.currentProd * 100.0 / p.totalProd)
	          : 0;

	std::cout << "주문: " << p.orderId << "  시료: " << p.sampleId << "\n"
	          << "진행: " << p.currentProd << "/" << p.totalProd << "개 ("
	          << pct << "%)\n"
	          << "완료 예정: " << timeBuf << "\n";
}

void MonitorView::showStockStatus(const Sample& sample, StockStatus status) const
{
	std::string tag;
	switch (status) {
	case StockStatus::SUFFICIENT: tag = "[여유]"; break;
	case StockStatus::SHORTAGE:   tag = "[부족]"; break;
	case StockStatus::DEPLETED:   tag = "[고갈]"; break;
	default:                      tag = "[알수없음]"; break;
	}
	std::cout << tag
		<< "  " << sample.getId()
		<< "  " << sample.getName()
		<< "  재고: " << sample.getStock() << "ea\n";
}