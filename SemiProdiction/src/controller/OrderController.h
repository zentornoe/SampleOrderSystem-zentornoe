#pragma once
#include <string>
#include <vector>
#include <queue>
#include "../model/Order.h"
#include "../model/ProductionJob.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

class OrderController {
public:
	explicit OrderController(IOrderRepository& orderRepo,
							 ISampleRepository& sampleRepo,
							 std::queue<ProductionJob>& prodQueue);

	std::string reserveOrder(const std::string& sampleId,
							 const std::string& customerName,
							 int quantity);
	void approveOrder(const std::string& orderId);
	void rejectOrder(const std::string& orderId);
	Order getOrder(const std::string& orderId) const;
	std::vector<Order> getAllOrders() const;

private:
	IOrderRepository& m_orderRepo;
	ISampleRepository& m_sampleRepo;
	std::queue<ProductionJob>& m_prodQueue;
};
