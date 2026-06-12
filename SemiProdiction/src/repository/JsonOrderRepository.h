#pragma once
#include "IOrderRepository.h"
#include <string>

class JsonOrderRepository : public IOrderRepository {
public:
	explicit JsonOrderRepository(const std::string& filePath);

	void save(const Order& order) override;
	Order findById(const std::string& id) const override;
	std::vector<Order> findAll() const override;
	std::vector<Order> findByStatus(OrderStatus status) const override;
	void update(const Order& order) override;
	bool exists(const std::string& id) const override;
	int getNextSequence() const override;

private:
	std::string m_filePath;
	void persist(const std::vector<Order>& orders);
	std::vector<Order> load() const;
};
