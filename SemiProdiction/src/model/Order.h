#pragma once
#include <string>
#include <ctime>
#include "OrderStatus.h"

class Order {
public:
	Order() = default;
	Order(const std::string& orderId, const std::string& sampleId,
		  const std::string& customerName, int quantity);

	const std::string& getOrderId() const;
	const std::string& getSampleId() const;
	const std::string& getCustomerName() const;
	int getQuantity() const;
	OrderStatus getStatus() const;
	long long getCreatedAt() const;

	void setStatus(OrderStatus status);

	bool isMonitored() const;
	static std::string generateOrderId(int sequence);

private:
	std::string m_orderId;
	std::string m_sampleId;
	std::string m_customerName;
	int m_quantity = 0;
	OrderStatus m_status = OrderStatus::RESERVED;
	long long m_createdAt = 0;
};
