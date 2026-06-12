#include "JsonProductionJobRepository.h"
#include "JsonHelper.h"
#include <sstream>
#include <iomanip>
#include <vector>

namespace {
	std::string toJson(const ProductionJob& j)
	{
		std::ostringstream oss;
		oss << std::setprecision(15);
		oss << "{"
			<< "\"orderId\":\""   << j.getOrderId()     << "\","
			<< "\"sampleId\":\""  << j.getSampleId()    << "\","
			<< "\"shortage\":"    << j.getShortage()    << ","
			<< "\"yield\":"       << j.getYield()       << ","
			<< "\"avgProdTime\":" << j.getAvgProdTime() << ","
			<< "\"startTime\":"   << j.getStartTime()
			<< "}";
		return oss.str();
	}

	ProductionJob fromJson(const std::string& obj)
	{
		return ProductionJob(
			jsonGetStr (obj, "orderId"),
			jsonGetStr (obj, "sampleId"),
			jsonGetInt (obj, "shortage"),
			jsonGetDbl (obj, "yield"),
			jsonGetDbl (obj, "avgProdTime"),
			jsonGetLong(obj, "startTime")
		);
	}
}

JsonProductionJobRepository::JsonProductionJobRepository(const std::string& filePath)
	: m_filePath(filePath)
{
}

void JsonProductionJobRepository::save(const std::queue<ProductionJob>& queue) const
{
	std::queue<ProductionJob> tmp = queue;
	std::vector<ProductionJob> jobs;
	while (!tmp.empty()) {
		jobs.push_back(tmp.front());
		tmp.pop();
	}
	jsonPersistArray(m_filePath, jobs, toJson);
}

std::queue<ProductionJob> JsonProductionJobRepository::load() const
{
	auto jobs = jsonLoadAll<ProductionJob>(m_filePath, fromJson);
	std::queue<ProductionJob> q;
	for (auto& j : jobs)
		q.push(j);
	return q;
}