#include "Sample.h"
#include <stdexcept>
#include <algorithm>

Sample::Sample(const std::string& id, const std::string& name,
	double avgProdTime, double yield, int stock, int reservedQty)
	: m_id(id), m_name(name), m_avgProdTime(avgProdTime),
	  m_yield(yield), m_stock(stock), m_reservedQty(reservedQty)
{
	if (yield <= 0.0 || yield > 1.0)
		throw std::invalid_argument("yield는 0 초과 1 이하여야 합니다.");
	if (avgProdTime <= 0.0)
		throw std::invalid_argument("avgProdTime은 0 초과여야 합니다.");
	if (stock < 0)
		throw std::invalid_argument("stock은 0 이상이어야 합니다.");
	if (reservedQty < 0)
		throw std::invalid_argument("reservedQty는 0 이상이어야 합니다.");
}

const std::string& Sample::getId() const { return m_id; }
const std::string& Sample::getName() const { return m_name; }
double Sample::getAvgProdTime() const { return m_avgProdTime; }
double Sample::getYield() const { return m_yield; }
int Sample::getStock() const { return m_stock; }
int Sample::getReservedQty() const { return m_reservedQty; }

int Sample::getAvailableStock() const
{
	return std::max(0, m_stock - m_reservedQty);
}

void Sample::addStock(int quantity)
{
	if (quantity <= 0)
		throw std::invalid_argument("추가 수량은 0 초과여야 합니다.");
	m_stock += quantity;
}

void Sample::reduceStock(int quantity)
{
	if (quantity > m_stock)
		throw std::runtime_error("재고 부족");
	m_stock -= quantity;
}

bool Sample::isStockEnough(int quantity) const
{
	return m_stock >= quantity;
}

void Sample::reserveQty(int quantity)
{
	if (quantity <= 0)
		throw std::invalid_argument("예약 수량은 0 초과여야 합니다.");
	m_reservedQty += quantity;
}

void Sample::releaseQty(int quantity)
{
	if (quantity > m_reservedQty)
		throw std::runtime_error("예약 수량 부족");
	m_reservedQty -= quantity;
}
