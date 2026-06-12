#pragma once
#include <gmock/gmock.h>
#include "src/repository/ISampleRepository.h"

class MockSampleRepository : public ISampleRepository {
public:
	MOCK_METHOD(void, save, (const Sample& sample), (override));
	MOCK_METHOD(Sample, findById, (const std::string& id), (const, override));
	MOCK_METHOD(std::vector<Sample>, findAll, (), (const, override));
	MOCK_METHOD((std::vector<Sample>), findByName, (const std::string& keyword), (const, override));
	MOCK_METHOD(void, update, (const Sample& sample), (override));
	MOCK_METHOD(bool, exists, (const std::string& id), (const, override));
};
