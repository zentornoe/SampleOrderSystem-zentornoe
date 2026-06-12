#include "ProductionJob.h"

ProductionJob::ProductionJob(const std::string& orderId, const std::string& sampleId,
	int shortage, double yield, double avgProdTime)
	: m_orderId(orderId), m_sampleId(sampleId),
	  m_shortage(shortage), m_yield(yield), m_avgProdTime(avgProdTime),
	  m_startTime(static_cast<long long>(std::time(nullptr)))
{
	m_actualProd = static_cast<int>(std::ceil(static_cast<double>(shortage) / (yield * 0.9)));
}

const std::string& ProductionJob::getOrderId() const { return m_orderId; }
const std::string& ProductionJob::getSampleId() const { return m_sampleId; }
int ProductionJob::getShortage() const { return m_shortage; }
double ProductionJob::getYield() const { return m_yield; }
double ProductionJob::getAvgProdTime() const { return m_avgProdTime; }
int ProductionJob::getActualProd() const { return m_actualProd; }

double ProductionJob::getTotalProdTime() const
{
	return m_avgProdTime * m_actualProd;
}

long long ProductionJob::getStartTime() const
{
	return m_startTime;
}

long long ProductionJob::getCompletionTime() const
{
	return m_startTime + static_cast<long long>(getTotalProdTime() * 60.0);
}

bool ProductionJob::isCompleted(long long nowSec) const
{
	return nowSec >= getCompletionTime();
}

int ProductionJob::getCurrentProd(long long nowSec) const
{
	long long elapsed = nowSec - m_startTime;
	if (elapsed <= 0)
		return 0;

	double totalSec = getTotalProdTime() * 60.0;
	double ratio = static_cast<double>(elapsed) / totalSec;
	double computed = m_actualProd * ratio;
	return static_cast<int>(std::min(static_cast<double>(m_actualProd), computed));
}
