#include "JsonSampleRepository.h"
#include "JsonHelper.h"
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace {
	constexpr const char* ERR_SAMPLE_NOT_FOUND = "시료를 찾을 수 없습니다: ";

	std::string toJson(const Sample& s)
	{
		std::ostringstream oss;
		oss << std::setprecision(15);
		oss << "{"
			<< "\"id\":\"" << s.getId() << "\","
			<< "\"name\":\"" << s.getName() << "\","
			<< "\"avgProdTime\":" << s.getAvgProdTime() << ","
			<< "\"yield\":" << s.getYield() << ","
			<< "\"stock\":" << s.getStock() << ","
			<< "\"reservedQty\":" << s.getReservedQty()
			<< "}";
		return oss.str();
	}

	Sample fromJson(const std::string& obj)
	{
		return Sample(
			jsonGetStr(obj, "id"),
			jsonGetStr(obj, "name"),
			jsonGetDbl(obj, "avgProdTime"),
			jsonGetDbl(obj, "yield"),
			jsonGetInt(obj, "stock"),
			jsonGetInt(obj, "reservedQty")
		);
	}
}

JsonSampleRepository::JsonSampleRepository(const std::string& filePath)
	: m_filePath(filePath)
{
}

void JsonSampleRepository::save(const Sample& sample)
{
	auto all = load();
	all.push_back(sample);
	persist(all);
}

Sample JsonSampleRepository::findById(const std::string& id) const
{
	for (const auto& s : load())
		if (s.getId() == id) return s;
	throw std::runtime_error(ERR_SAMPLE_NOT_FOUND + id);
}

std::vector<Sample> JsonSampleRepository::findAll() const
{
	return load();
}

std::vector<Sample> JsonSampleRepository::findByName(const std::string& keyword) const
{
	std::vector<Sample> result;
	for (const auto& s : load())
		if (s.getName().find(keyword) != std::string::npos)
			result.push_back(s);
	return result;
}

void JsonSampleRepository::update(const Sample& sample)
{
	auto all = load();
	for (auto& s : all) {
		if (s.getId() == sample.getId()) {
			s = sample;
			persist(all);
			return;
		}
	}
	throw std::runtime_error(ERR_SAMPLE_NOT_FOUND + sample.getId());
}

bool JsonSampleRepository::exists(const std::string& id) const
{
	for (const auto& s : load())
		if (s.getId() == id) return true;
	return false;
}

void JsonSampleRepository::persist(const std::vector<Sample>& samples)
{
	jsonPersistArray(m_filePath, samples, toJson);
}

std::vector<Sample> JsonSampleRepository::load() const
{
	return jsonLoadAll<Sample>(m_filePath, fromJson);
}
