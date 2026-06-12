#pragma once
#include <gmock/gmock.h>
#include "src/repository/IOrderRepository.h"

class MockOrderRepository : public IOrderRepository {
public:
	MOCK_METHOD(void, save, (const Order& order), (override));
	MOCK_METHOD(Order, findById, (const std::string& id), (const, override));
	MOCK_METHOD(std::vector<Order>, findAll, (), (const, override));
	MOCK_METHOD((std::vector<Order>), findByStatus, (OrderStatus status), (const, override));
	MOCK_METHOD(void, update, (const Order& order), (override));
	MOCK_METHOD(bool, exists, (const std::string& id), (const, override));
	MOCK_METHOD(int, getNextSequence, (), (const, override));
};
