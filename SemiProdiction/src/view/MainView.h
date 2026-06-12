#pragma once
#include <string>

class MainView {
public:
	void showMainMenu() const;
	int getMenuChoice() const;
	void showError(const std::string& message) const;
};
