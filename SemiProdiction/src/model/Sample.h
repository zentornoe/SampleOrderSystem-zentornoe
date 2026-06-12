#pragma once
#include <string>

class Sample {
public:
	Sample() = default;
	Sample(const std::string& id, const std::string& name,
		   double avgProdTime, double yield, int stock);

	const std::string& getId() const;
	const std::string& getName() const;
	double getAvgProdTime() const;
	double getYield() const;
	int getStock() const;

	void addStock(int quantity);
	void reduceStock(int quantity);
	bool isStockEnough(int quantity) const;

private:
	std::string m_id;
	std::string m_name;
	double m_avgProdTime = 0.0;
	double m_yield = 0.0;
	int m_stock = 0;
};
