#include "SampleView.h"
#include <iostream>
#include <iomanip>

void SampleView::showSampleList(const std::vector<Sample>& samples) const
{
	if (samples.empty()) {
		std::cout << "등록된 시료가 없습니다.\n";
		return;
	}
	std::cout << std::left
		<< std::setw(12) << "ID"
		<< std::setw(22) << "이름"
		<< std::setw(16) << "평균생산시간(분)"
		<< std::setw(8)  << "수율"
		<< std::setw(8)  << "재고"
		<< "가용재고"
		<< "\n"
		<< std::string(74, '-') << "\n";
	for (const auto& s : samples) {
		std::cout << std::left
			<< std::setw(12) << s.getId()
			<< std::setw(22) << s.getName()
			<< std::setw(16) << s.getAvgProdTime()
			<< std::setw(8)  << s.getYield()
			<< std::setw(8)  << s.getStock()
			<< s.getAvailableStock()
			<< "\n";
	}
}

void SampleView::showSample(const Sample& sample) const
{
	std::cout << "ID        : " << sample.getId()            << "\n"
	          << "이름      : " << sample.getName()          << "\n"
	          << "생산시간  : " << sample.getAvgProdTime()   << " 분\n"
	          << "수율      : " << sample.getYield()         << "\n"
	          << "재고      : " << sample.getStock()         << "\n"
	          << "가용재고  : " << sample.getAvailableStock() << "\n";
}

void SampleView::showRegisterSuccess(const std::string& id) const
{
	std::cout << "[등록 완료] " << id << "\n";
}

void SampleView::showAddStockSuccess(const std::string& id, int quantity) const
{
	std::cout << "[입고 완료] " << id << " + " << quantity << "개\n";
}

std::string SampleView::inputId() const
{
	std::cout << "시료 ID 입력: ";
	std::string val;
	std::cin >> val;
	return val;
}

std::string SampleView::inputName() const
{
	std::cout << "시료 이름 입력: ";
	std::string val;
	std::getline(std::cin >> std::ws, val);
	return val;
}

double SampleView::inputYield() const
{
	std::cout << "수율 입력 (0 초과 ~ 1 이하): ";
	double val;
	std::cin >> val;
	return val;
}

double SampleView::inputAvgProdTime() const
{
	std::cout << "평균 생산시간 입력 (분): ";
	double val;
	std::cin >> val;
	return val;
}

int SampleView::inputStock() const
{
	std::cout << "초기 재고 입력: ";
	int val;
	std::cin >> val;
	return val;
}

int SampleView::inputQuantity() const
{
	std::cout << "수량 입력: ";
	int val;
	std::cin >> val;
	return val;
}
