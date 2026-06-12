#include <iostream>
#include <stdexcept>
#include "controller/SampleController.h"
#include "view/SampleView.h"
#include "repository/JsonSampleRepository.h"

// ─────────────────────────────────────────────
//  시료 관리 서브메뉴
// ─────────────────────────────────────────────
static void runSampleMenu(SampleController& ctrl, SampleView& view)
{
	while (true) {
		std::cout << "\n=== 시료 관리 ===\n"
		          << "1. 시료 등록\n"
		          << "2. 시료 전체 조회\n"
		          << "3. 이름으로 검색\n"
		          << "0. 돌아가기\n"
		          << "선택: ";
		int choice;
		if (!(std::cin >> choice)) { std::cin.clear(); std::cin.ignore(1024, '\n'); continue; }

		if (choice == 0) break;

		switch (choice) {
		case 1: {
			std::string id   = view.inputId();
			std::string name = view.inputName();
			double prodTime  = view.inputAvgProdTime();
			double yield     = view.inputYield();
			int    stock     = view.inputStock();
			try {
				ctrl.registerSample(id, name, prodTime, yield, stock);
				view.showRegisterSuccess(id);
			} catch (const std::exception& e) {
				std::cout << "[오류] " << e.what() << "\n";
			}
			break;
		}
		case 2:
			view.showSampleList(ctrl.getAllSamples());
			break;
		case 3: {
			std::cout << "검색 키워드: ";
			std::string keyword;
			std::getline(std::cin >> std::ws, keyword);
			view.showSampleList(ctrl.searchByName(keyword));
			break;
		}
		default:
			std::cout << "[오류] 없는 메뉴입니다.\n";
		}
	}
}

// ─────────────────────────────────────────────
//  메인 메뉴
// ─────────────────────────────────────────────
int main()
{
	JsonSampleRepository sampleRepo("data/samples.json");
	SampleController     sampleCtrl(sampleRepo);
	SampleView           sampleView;

	while (true) {
		std::cout << "\n========================================\n"
		          << "  S-Semi 반도체 시료 생산주문관리 시스템\n"
		          << "========================================\n"
		          << "1. 시료 관리\n"
		          << "2. 시료 주문         (미구현)\n"
		          << "3. 주문 승인/거절    (미구현)\n"
		          << "4. 모니터링          (미구현)\n"
		          << "5. 생산라인 조회     (미구현)\n"
		          << "6. 출고 처리         (미구현)\n"
		          << "0. 종료\n"
		          << "선택: ";

		int choice;
		if (!(std::cin >> choice)) { std::cin.clear(); std::cin.ignore(1024, '\n'); continue; }

		if (choice == 0) {
			std::cout << "시스템을 종료합니다.\n";
			break;
		}

		switch (choice) {
		case 1: runSampleMenu(sampleCtrl, sampleView); break;
		default: std::cout << "[오류] 없는 메뉴입니다.\n"; break;
		}
	}

	return 0;
}
