#pragma once
#include <string>
#include <ctime>

class ProductionJob {
public:
	ProductionJob() = default;
	ProductionJob(const std::string& orderId, const std::string& sampleId,
				  int shortage, double yield, double avgProdTime);

	const std::string& getOrderId() const;
	const std::string& getSampleId() const;
	int getShortage() const;
	double getYield() const;
	double getAvgProdTime() const;
	int getActualProd() const;
	double getTotalProdTime() const;
	long long getStartTime() const;
	long long getCompletionTime() const;
	bool isCompleted(long long nowSec) const;
	int getCurrentProd(long long nowSec) const;

private:
	std::string m_orderId;
	std::string m_sampleId;
	int m_shortage = 0;
	double m_yield = 0.0;
	double m_avgProdTime = 0.0;
	int m_actualProd = 0;
	long long m_startTime = 0;
};
