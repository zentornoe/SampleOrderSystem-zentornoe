#pragma once
#include <string>
#include <vector>
#include "../model/Sample.h"

class ISampleRepository {
public:
	virtual ~ISampleRepository() = default;

	virtual void save(const Sample& sample) = 0;
	virtual Sample findById(const std::string& id) const = 0;
	virtual std::vector<Sample> findAll() const = 0;
	virtual std::vector<Sample> findByName(const std::string& keyword) const = 0;
	virtual void update(const Sample& sample) = 0;
	virtual bool exists(const std::string& id) const = 0;
};
