#pragma once
#include <string>
#include <vector>
#include "../model/Order.h"

class IOrderRepository {
public:
	virtual ~IOrderRepository() = default;

	virtual void save(const Order& order) = 0;
	virtual Order findById(const std::string& id) const = 0;
	virtual std::vector<Order> findAll() const = 0;
	virtual std::vector<Order> findByStatus(OrderStatus status) const = 0;
	virtual void update(const Order& order) = 0;
	virtual bool exists(const std::string& id) const = 0;
	virtual int getNextSequence() const = 0;
};
