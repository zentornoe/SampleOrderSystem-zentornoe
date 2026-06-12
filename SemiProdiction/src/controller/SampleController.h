#pragma once
#include <string>
#include <vector>
#include "../model/Sample.h"
#include "../repository/ISampleRepository.h"

class SampleController {
public:
	explicit SampleController(ISampleRepository& repo);

	void registerSample(const std::string& id, const std::string& name,
						double avgProdTime, double yield, int stock);
	Sample getSample(const std::string& id) const;
	std::vector<Sample> getAllSamples() const;
	std::vector<Sample> searchByName(const std::string& keyword) const;
	void addStock(const std::string& id, int quantity);

private:
	ISampleRepository& m_repo;
};
