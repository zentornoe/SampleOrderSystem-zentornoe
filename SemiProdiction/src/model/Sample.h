#pragma once
#include <string>

class Sample {
public:
	Sample(const std::string& id, const std::string& name,
		   double avgProdTime, double yield, int stock,
		   int reservedQty = 0);

	const std::string& getId() const;
	const std::string& getName() const;
	double getAvgProdTime() const;
	double getYield() const;
	int getStock() const;
	int getReservedQty() const;
	int getAvailableStock() const;	// stock - reservedQty (최솟값 0)

	void addStock(int quantity);
	void reduceStock(int quantity);
	bool isStockEnough(int quantity) const;

	void reserveQty(int quantity);		// 승인 시 예약 수량 증가
	void releaseQty(int quantity);		// 출고 시 예약 수량 감소

private:
	Sample() = default;				// Repository 역직렬화 전용

	std::string m_id;
	std::string m_name;
	double m_avgProdTime = 0.0;
	double m_yield = 0.0;
	int m_stock = 0;
	int m_reservedQty = 0;
};
