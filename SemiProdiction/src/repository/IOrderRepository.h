#pragma once
#include <string>
#include <vector>
#include "../model/Order.h"

class IOrderRepository {
public:
	virtual ~IOrderRepository() = default;

	virtual void save(const Order& order) = 0;
	virtual Order findById(const std::string& id) = 0;
	virtual std::vector<Order> findAll() = 0;
	virtual std::vector<Order> findByStatus(OrderStatus status) = 0;
	virtual void update(const Order& order) = 0;
	virtual bool exists(const std::string& id) = 0;
	virtual int getNextSequence() = 0;
};
