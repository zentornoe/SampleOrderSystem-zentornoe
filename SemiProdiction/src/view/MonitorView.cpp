#include "MonitorView.h"
#include "../model/OrderStatus.h"
#include <iostream>
#include <string>
#include <iomanip>

void MonitorView::showOrderSummary(const OrderSummary& summary) const
{
	std::cout << "=== 주문 현황 ===\n"
		<< std::string(64, '-') << "\n"
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
		<< std::string(64, '-') << "\n"
		<< "주문번호 / 시료 / 고객명 / 수량 / 상태\n"
		<< std::string(64, '-') << "\n";
	for (const auto& order : orders) {
		std::cout << order.getOrderId()
			<< " / " << order.getSampleId()
			<< " / " << order.getCustomerName()
			<< " / " << order.getQuantity()
			<< " / " << toString(order.getStatus())
			<< "\n";
	}
}

void MonitorView::showStockStatus(const Sample& sample, StockStatus status) const
{
	std::string tag;
	switch (status) {
	case StockStatus::SUFFICIENT: tag = "[여유]"; break;
	case StockStatus::SHORTAGE:   tag = "[부족]"; break;
	case StockStatus::DEPLETED:   tag = "[고갈]"; break;
	}
	std::cout << tag
		<< "  " << sample.getId()
		<< "  " << sample.getName()
		<< "  재고: " << sample.getStock() << "ea\n";
}