#include "Sample.h"
#include <stdexcept>

Sample::Sample(const std::string& id, const std::string& name,
	double avgProdTime, double yield, int stock)
	: m_id(id), m_name(name), m_avgProdTime(avgProdTime), m_yield(yield), m_stock(stock)
{
	if (yield <= 0.0 || yield > 1.0)
		throw std::invalid_argument("yield는 0 초과 1 이하여야 합니다.");
	if (avgProdTime <= 0.0)
		throw std::invalid_argument("avgProdTime은 0 초과여야 합니다.");
	if (stock < 0)
		throw std::invalid_argument("stock은 0 이상이어야 합니다.");
}

const std::string& Sample::getId() const { return m_id; }
const std::string& Sample::getName() const { return m_name; }
double Sample::getAvgProdTime() const { return m_avgProdTime; }
double Sample::getYield() const { return m_yield; }
int Sample::getStock() const { return m_stock; }

void Sample::addStock(int qty)
{
	if (qty <= 0)
		throw std::invalid_argument("추가 수량은 0 초과여야 합니다.");
	m_stock += qty;
}

void Sample::reduceStock(int qty)
{
	if (qty > m_stock)
		throw std::runtime_error("재고 부족");
	m_stock -= qty;
}

bool Sample::isStockEnough(int qty) const
{
	return m_stock >= qty;
}
