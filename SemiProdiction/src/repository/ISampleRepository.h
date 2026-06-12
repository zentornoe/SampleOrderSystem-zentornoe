#pragma once
#include <string>
#include <vector>
#include "../model/Sample.h"

class ISampleRepository {
public:
	virtual ~ISampleRepository() = default;

	virtual void save(const Sample& sample) = 0;
	virtual Sample findById(const std::string& id) = 0;
	virtual std::vector<Sample> findAll() = 0;
	virtual std::vector<Sample> findByName(const std::string& keyword) = 0;
	virtual void update(const Sample& sample) = 0;
	virtual bool exists(const std::string& id) = 0;
};
