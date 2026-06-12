#include "MainView.h"
#include <iostream>
#include <limits>

void MainView::showMainMenu() const {
	std::cout << "\n==============================\n";
	std::cout << "  S-Semi 생산주문관리 시스템\n";
	std::cout << "==============================\n";
	std::cout << "  1. 시료 관리\n";
	std::cout << "  2. 주문 접수\n";
	std::cout << "  3. 주문 승인 / 거절\n";
	std::cout << "  4. 생산 현황\n";
	std::cout << "  5. 모니터링\n";
	std::cout << "  6. 출고 처리\n";
	std::cout << "  0. 종료\n";
	std::cout << "------------------------------\n";
	std::cout << "선택: ";
}

int MainView::getMenuChoice() const {
	int choice = -1;
	std::cin >> choice;
	if (std::cin.fail()) {
		std::cin.clear();
		std::cin.ignore(10000, '\n');
		return -1;
	}
	return choice;
}

void MainView::showError(const std::string& message) const {
	std::cout << "[오류] " << message << "\n";
}

void MainView::showExitMessage() const {
	std::cout << "시스템을 종료합니다.\n";
}

void MainView::showSampleSubMenu() const {
	std::cout << "\n=== 시료 관리 ===\n"
	          << "  1. 시료 등록\n"
	          << "  2. 전체 조회\n"
	          << "  3. 이름 검색\n"
	          << "  0. 돌아가기\n"
	          << "선택: ";
}

int MainView::getSampleSubMenuChoice() const {
	int choice = -1;
	if (!(std::cin >> choice)) {
		std::cin.clear();
		std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
		return -1;
	}
	return choice;
}