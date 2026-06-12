#pragma once
#include <vector>
#include <string>
#include "../model/Order.h"

class OrderView {
public:
	void showOrderList(const std::vector<Order>& orders) const;
	void showOrder(const Order& order) const;
	void showReserveSuccess(const std::string& orderId) const;
	void showApproveSuccess(const std::string& orderId) const;
	void showRejectSuccess(const std::string& orderId) const;

	std::string inputSampleId() const;
	std::string inputCustomerName() const;
	int inputQuantity() const;
	std::string inputOrderId() const;
};
