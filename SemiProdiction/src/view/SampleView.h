#pragma once
#include <vector>
#include <string>
#include "../model/Sample.h"

class SampleView {
public:
	void showSampleList(const std::vector<Sample>& samples) const;
	void showRegisterSuccess(const std::string& id) const;
	void showAddStockSuccess(const std::string& id, int quantity) const;

	std::string inputId() const;
	std::string inputName() const;
	double inputYield() const;
	double inputAvgProdTime() const;
	int inputStock() const;
	std::string inputSearchKeyword() const;
};
