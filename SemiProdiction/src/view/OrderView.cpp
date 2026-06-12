#include "OrderView.h"
#include "../model/OrderStatus.h"
#include <iostream>
#include <limits>
#include <string>

namespace {
	constexpr int TABLE_WIDTH = 64;
}

void OrderView::showOrderList(const std::vector<Order>& orders) const
{
	if (orders.empty()) {
		std::cout << "주문이 없습니다.\n";
		return;
	}

	std::cout << "주문번호 | 시료 ID | 고객명 | 수량 | 상태" << "\n"
		<< std::string(TABLE_WIDTH, '-') << "\n";
	for (const auto& order : orders) {
		std::cout << order.getOrderId()
			<< " | " << order.getSampleId()
			<< " | " << order.getCustomerName()
			<< " | " << order.getQuantity()
			<< " | " << toString(order.getStatus())
			<< "\n";
	}
}

void OrderView::showOrder(const Order& order) const
{
	std::cout << "주문번호: " << order.getOrderId()      << "\n"
		<< "시료 ID: "  << order.getSampleId()           << "\n"
		<< "고객명: "   << order.getCustomerName()        << "\n"
		<< "수량: "     << order.getQuantity()            << "\n"
		<< "상태: "     << toString(order.getStatus())    << "\n"
		<< "접수시각: " << order.getCreatedAt()           << "\n";
}

void OrderView::showReserveSuccess(const std::string& orderId) const
{
	std::cout << "[주문 접수 완료] " << orderId << "\n";
}

void OrderView::showApproveSuccess(const std::string& orderId) const
{
	std::cout << "[주문 승인 완료] " << orderId << "\n";
}

void OrderView::showRejectSuccess(const std::string& orderId) const
{
	std::cout << "[주문 거절 완료] " << orderId << "\n";
}

void OrderView::showReleaseSuccess(const std::string& orderId) const
{
	std::cout << "[출고 완료] " << orderId << "\n";
}

void OrderView::showApproveRejectMenu() const
{
	std::cout << "  1. 승인\n"
	          << "  2. 거절\n"
	          << "선택: ";
}

int OrderView::inputApproveRejectChoice() const
{
	int choice = -1;
	if (!(std::cin >> choice)) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		return -1;
	}
	return choice;
}

std::string OrderView::inputSampleId() const
{
	std::string value;
	std::cout << "시료 ID: ";
	std::cin >> value;
	return value;
}

std::string OrderView::inputCustomerName() const
{
	std::string value;
	std::cout << "고객명: ";
	std::getline(std::cin >> std::ws, value);
	return value;
}

int OrderView::inputQuantity() const
{
	int value;
	std::cout << "수량: ";
	std::cin >> value;
	return value;
}

std::string OrderView::inputOrderId() const
{
	std::string value;
	std::cout << "주문번호: ";
	std::cin >> value;
	return value;
}