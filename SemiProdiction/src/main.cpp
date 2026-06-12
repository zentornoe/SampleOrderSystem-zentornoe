#include <queue>
#include <ctime>
#include <iostream>
#include <limits>
#include <stdexcept>
#include "model/ProductionJob.h"
#include "repository/JsonSampleRepository.h"
#include "repository/JsonOrderRepository.h"
#include "repository/JsonProductionJobRepository.h"
#include "controller/SampleController.h"
#include "controller/OrderController.h"
#include "controller/ProductionController.h"
#include "controller/MonitoringController.h"
#include "controller/ReleaseController.h"
#include "view/MainView.h"
#include "view/SampleView.h"
#include "view/OrderView.h"
#include "view/ProductionView.h"
#include "view/MonitorView.h"

// ─────────────────────────────────────────────
//  핸들러 전방선언
// ─────────────────────────────────────────────
static void handleSampleMenu(SampleController& ctrl, SampleView& view, MainView& mainView);
static void handleOrderReserve(OrderController& orderCtrl, OrderView& orderView);
static void handleOrderApproveReject(OrderController& orderCtrl, OrderView& orderView, MainView& mainView, long long nowSec);
static void handleProduction(ProductionController& prodCtrl, ProductionView& prodView, long long nowSec);
static void handleMonitoring(MonitoringController& monCtrl, SampleController& sampleCtrl, MonitorView& monView, long long nowSec);
static void handleRelease(ReleaseController& releaseCtrl, OrderView& orderView);

// ─────────────────────────────────────────────
//  시료 관리 서브메뉴
// ─────────────────────────────────────────────
static void handleSampleMenu(SampleController& ctrl, SampleView& view, MainView& mainView)
{
	while (true) {
		std::cout << "\n=== 시료 관리 ===\n"
		          << "  1. 시료 등록\n"
		          << "  2. 전체 조회\n"
		          << "  3. 이름 검색\n"
		          << "  0. 돌아가기\n"
		          << "선택: ";
		int choice = -1;
		if (!(std::cin >> choice)) {
			std::cin.clear();
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
			continue;
		}
		if (choice == 0) break;
		switch (choice) {
		case 1: {
			std::string id      = view.inputId();
			std::string name    = view.inputName();
			double prodTime     = view.inputAvgProdTime();
			double yield        = view.inputYield();
			int    stock        = view.inputStock();
			try {
				ctrl.registerSample(id, name, prodTime, yield, stock);
				view.showRegisterSuccess(id);
			} catch (const std::exception& e) {
				mainView.showError(e.what());
			}
			break;
		}
		case 2:
			view.showSampleList(ctrl.getAllSamples());
			break;
		case 3: {
			std::string keyword = view.inputSearchKeyword();
			view.showSampleList(ctrl.searchByName(keyword));
			break;
		}
		default:
			mainView.showError("없는 메뉴입니다.");
			break;
		}
	}
}

// ─────────────────────────────────────────────
//  주문 접수
// ─────────────────────────────────────────────
static void handleOrderReserve(OrderController& orderCtrl, OrderView& orderView)
{
	std::string sampleId     = orderView.inputSampleId();
	std::string customerName = orderView.inputCustomerName();
	int quantity             = orderView.inputQuantity();
	std::string orderId      = orderCtrl.reserveOrder(sampleId, customerName, quantity);
	orderView.showReserveSuccess(orderId);
}

// ─────────────────────────────────────────────
//  주문 승인 / 거절
// ─────────────────────────────────────────────
static void handleOrderApproveReject(OrderController& orderCtrl, OrderView& orderView, MainView& mainView, long long nowSec)
{
	std::vector<Order> reserved = orderCtrl.getReservedOrders();
	orderView.showOrderList(reserved);
	if (reserved.empty()) return;

	std::string orderId = orderView.inputOrderId();
	orderView.showApproveRejectMenu();
	int choice = orderView.inputApproveRejectChoice();

	switch (choice) {
	case 1:
		orderCtrl.approveOrder(orderId, nowSec);
		orderView.showApproveSuccess(orderId);
		break;
	case 2:
		orderCtrl.rejectOrder(orderId);
		orderView.showRejectSuccess(orderId);
		break;
	default:
		mainView.showError("없는 메뉴입니다.");
		break;
	}
}

// ─────────────────────────────────────────────
//  생산 현황
// ─────────────────────────────────────────────
static void handleProduction(ProductionController& prodCtrl, ProductionView& prodView, long long nowSec)
{
	if (prodCtrl.hasJob()) {
		prodView.showCurrentJob(prodCtrl.peekNextJob(), nowSec);
	}
	prodView.showProductionQueue(prodCtrl.getQueue(), nowSec);
}

// ─────────────────────────────────────────────
//  모니터링
// ─────────────────────────────────────────────
static void handleMonitoring(MonitoringController& monCtrl, SampleController& sampleCtrl, MonitorView& monView, long long nowSec)
{
	monView.showOrderSummary(monCtrl.getOrderSummary());
	monView.showActiveOrders(monCtrl.getActiveOrders());
	monView.showProductionProgress(monCtrl.getProductionProgress(nowSec));
	for (const auto& s : sampleCtrl.getAllSamples()) {
		monView.showStockStatus(s, monCtrl.getStockStatus(s.getId()));
	}
}

// ─────────────────────────────────────────────
//  출고 처리
// ─────────────────────────────────────────────
static void handleRelease(ReleaseController& releaseCtrl, OrderView& orderView)
{
	std::vector<Order> confirmed = releaseCtrl.getConfirmedOrders();
	orderView.showOrderList(confirmed);
	if (confirmed.empty()) return;

	std::string orderId = orderView.inputOrderId();
	releaseCtrl.releaseOrder(orderId);
	orderView.showReleaseSuccess(orderId);
}

// ─────────────────────────────────────────────
//  메인 진입점
// ─────────────────────────────────────────────
int main()
{
	// Repository
	JsonSampleRepository        sampleRepo("data/samples.json");
	JsonOrderRepository         orderRepo ("data/orders.json");
	JsonProductionJobRepository jobRepo   ("data/production_jobs.json");

	// 공유 생산 큐 — 재시작 시 이전 상태 복원
	std::queue<ProductionJob> prodQueue = jobRepo.load();

	// Controller 조립
	SampleController     sampleCtrl(sampleRepo);
	OrderController      orderCtrl(orderRepo, sampleRepo, prodQueue);
	ProductionController prodCtrl(orderRepo, sampleRepo, prodQueue);
	MonitoringController monCtrl(orderRepo, sampleRepo, prodQueue);
	ReleaseController    releaseCtrl(orderRepo, sampleRepo);

	// View 조립
	MainView       mainView;
	SampleView     sampleView;
	OrderView      orderView;
	ProductionView prodView;
	MonitorView    monView;

	while (true) {
		long long nowSec = static_cast<long long>(std::time(nullptr));
		prodCtrl.tick(nowSec);

		mainView.showMainMenu();
		int choice = mainView.getMenuChoice();

		try {
			switch (choice) {
			case 0:
				jobRepo.save(prodQueue);
				std::cout << "시스템을 종료합니다.\n";
				return 0;
			case 1:
				handleSampleMenu(sampleCtrl, sampleView, mainView);
				break;
			case 2:
				handleOrderReserve(orderCtrl, orderView);
				break;
			case 3:
				handleOrderApproveReject(orderCtrl, orderView, mainView, nowSec);
				break;
			case 4:
				handleProduction(prodCtrl, prodView, nowSec);
				break;
			case 5:
				handleMonitoring(monCtrl, sampleCtrl, monView, nowSec);
				break;
			case 6:
				handleRelease(releaseCtrl, orderView);
				break;
			default:
				mainView.showError("없는 메뉴입니다.");
				break;
			}
		} catch (const std::exception& e) {
			mainView.showError(e.what());
		}
		// tick() 완료 또는 approveOrder() 로 큐가 변경됐을 수 있으므로 매 루프 저장
		jobRepo.save(prodQueue);
	}

	return 0;
}