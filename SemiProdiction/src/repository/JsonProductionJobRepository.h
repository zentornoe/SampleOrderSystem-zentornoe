#pragma once
#include <queue>
#include <string>
#include "../model/ProductionJob.h"

class JsonProductionJobRepository {
public:
	explicit JsonProductionJobRepository(const std::string& filePath);

	void                      save(const std::queue<ProductionJob>& queue) const;
	std::queue<ProductionJob> load() const;

private:
	std::string m_filePath;
};