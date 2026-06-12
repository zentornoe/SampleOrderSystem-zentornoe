#pragma once
#include "ISampleRepository.h"
#include <string>

class JsonSampleRepository : public ISampleRepository {
public:
	explicit JsonSampleRepository(const std::string& filePath);

	void save(const Sample& sample) override;
	Sample findById(const std::string& id) const override;
	std::vector<Sample> findAll() const override;
	std::vector<Sample> findByName(const std::string& keyword) const override;
	void update(const Sample& sample) override;
	bool exists(const std::string& id) const override;

private:
	std::string m_filePath;
	void persist(const std::vector<Sample>& samples);
	std::vector<Sample> load() const;
};
