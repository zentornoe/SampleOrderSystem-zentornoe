#pragma once
#include <vector>
#include "../model/Order.h"
#include "../model/Sample.h"
#include "../model/MonitoringSummary.h"

class MonitorView {
public:
	void showOrderSummary(const OrderSummary& summary) const;
	void showActiveOrders(const std::vector<Order>& orders) const;
	void showStockStatus(const Sample& sample, StockStatus status) const;
};
