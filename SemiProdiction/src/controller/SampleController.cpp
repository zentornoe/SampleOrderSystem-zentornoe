#include "SampleController.h"
#include <stdexcept>

SampleController::SampleController(ISampleRepository& repo)
	: m_repo(repo)
{
}

void SampleController::registerSample(const std::string& id, const std::string& name,
	double avgProdTime, double yield, int stock)
{
	if (m_repo.exists(id))
		throw std::runtime_error("이미 등록된 시료 ID: " + id);
	// Sample 생성자가 yield, avgProdTime, stock 유효성 검사 → invalid_argument 전파
	m_repo.save(Sample(id, name, avgProdTime, yield, stock));
}

Sample SampleController::getSample(const std::string& id) const
{
	return m_repo.findById(id);
}

std::vector<Sample> SampleController::getAllSamples() const
{
	return m_repo.findAll();
}

std::vector<Sample> SampleController::searchByName(const std::string& keyword) const
{
	return m_repo.findByName(keyword);
}

void SampleController::addStock(const std::string& id, int quantity)
{
	Sample s = m_repo.findById(id);
	s.addStock(quantity);
	m_repo.update(s);
}
