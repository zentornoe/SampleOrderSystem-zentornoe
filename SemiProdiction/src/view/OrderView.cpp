#include "OrderView.h"
#include "../model/OrderStatus.h"
#include <iostream>

void OrderView::showOrderList(const std::vector<Order>& orders) const
{
	if (orders.empty()) {
		std::cout << "주문이 없습니다." << std::endl;
		return;
	}

	std::cout << "주문번호 | 시료 ID | 고객명 | 수량 | 상태" << std::endl;
	for (const auto& order : orders) {
		std::cout << order.getOrderId()
			<< " | " << order.getSampleId()
			<< " | " << order.getCustomerName()
			<< " | " << order.getQuantity()
			<< " | " << toString(order.getStatus())
			<< std::endl;
	}
}

void OrderView::showOrder(const Order& order) const
{
	std::cout << "주문번호: " << order.getOrderId() << std::endl;
	std::cout << "시료 ID: "  << order.getSampleId() << std::endl;
	std::cout << "고객명: "   << order.getCustomerName() << std::endl;
	std::cout << "수량: "     << order.getQuantity() << std::endl;
	std::cout << "상태: "     << toString(order.getStatus()) << std::endl;
	std::cout << "접수시각: " << order.getCreatedAt() << std::endl;
}

void OrderView::showReserveSuccess(const std::string& orderId) const
{
	std::cout << "[주문 접수 완료] " << orderId << std::endl;
}

void OrderView::showApproveSuccess(const std::string& orderId) const
{
	std::cout << "[주문 승인 완료] " << orderId << std::endl;
}

void OrderView::showRejectSuccess(const std::string& orderId) const
{
	std::cout << "[주문 거절 완료] " << orderId << std::endl;
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