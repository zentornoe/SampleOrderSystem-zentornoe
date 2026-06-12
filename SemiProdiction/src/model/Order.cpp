#include "Order.h"
#include <ctime>
#include <cstdio>
#include <stdexcept>

Order::Order(const std::string& orderId, const std::string& sampleId,
	const std::string& customerName, int quantity)
	: m_orderId(orderId), m_sampleId(sampleId),
	  m_customerName(customerName), m_quantity(quantity),
	  m_status(OrderStatus::RESERVED), m_createdAt(std::time(nullptr))
{
	if (quantity <= 0)
		throw std::invalid_argument("수량은 0 초과여야 합니다.");
	if (customerName.empty())
		throw std::invalid_argument("고객명은 비어 있을 수 없습니다.");
}

Order::Order(const std::string& orderId, const std::string& sampleId,
	const std::string& customerName, int quantity,
	OrderStatus status, long long createdAt)
	: m_orderId(orderId), m_sampleId(sampleId),
	  m_customerName(customerName), m_quantity(quantity),
	  m_status(status), m_createdAt(createdAt)
{
}

const std::string& Order::getOrderId() const { return m_orderId; }
const std::string& Order::getSampleId() const { return m_sampleId; }
const std::string& Order::getCustomerName() const { return m_customerName; }
int Order::getQuantity() const { return m_quantity; }
OrderStatus Order::getStatus() const { return m_status; }
long long Order::getCreatedAt() const { return m_createdAt; }

void Order::setStatus(OrderStatus status) { m_status = status; }

bool Order::isMonitored() const
{
	return m_status != OrderStatus::REJECTED;
}

std::string Order::generateOrderId(int sequence)
{
	std::time_t now = std::time(nullptr);
	std::tm tm_buf;
#ifdef _WIN32
	localtime_s(&tm_buf, &now);
#else
	localtime_r(&now, &tm_buf);
#endif
	char dateBuf[16];
	std::strftime(dateBuf, sizeof(dateBuf), "%Y%m%d", &tm_buf);

	char seqBuf[8];
	std::snprintf(seqBuf, sizeof(seqBuf), "%04d", sequence);

	return std::string("ORD-") + dateBuf + "-" + seqBuf;
}
