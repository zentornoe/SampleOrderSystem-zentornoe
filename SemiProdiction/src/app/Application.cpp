#include "Application.h"
#include <ctime>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

Application::Application()
	: m_sampleRepo("data/samples.json")
	, m_orderRepo("data/orders.json")
	, m_jobRepo("data/production_jobs.json")
	, m_prodQueue(m_jobRepo.load())
	, m_sampleCtrl(m_sampleRepo)
	, m_orderCtrl(m_orderRepo, m_sampleRepo, m_prodQueue)
	, m_prodCtrl(m_orderRepo, m_sampleRepo, m_prodQueue)
	, m_monCtrl(m_orderRepo, m_sampleRepo, m_prodQueue)
	, m_releaseCtrl(m_orderRepo, m_sampleRepo)
{}

void Application::run()
{
	while (true) {
		long long nowSec = static_cast<long long>(std::time(nullptr));
		m_prodCtrl.tick(nowSec);

		m_mainView.showMainMenu();
		int choice = m_mainView.getMenuChoice();

		try {
			switch (choice) {
			case 0:
				m_jobRepo.save(m_prodQueue);
				m_mainView.showExitMessage();
				return;
			case 1: handleSampleMenu();               break;
			case 2: handleOrderReserve();              break;
			case 3: handleOrderApproveReject(nowSec); break;
			case 4: handleProduction(nowSec);          break;
			case 5: handleMonitoring(nowSec);          break;
			case 6: handleRelease();                   break;
			default: m_mainView.showError("없는 메뉴입니다."); break;
			}
		} catch (const std::exception& e) {
			m_mainView.showError(e.what());
		}
		m_jobRepo.save(m_prodQueue);
	}
}

void Application::handleSampleMenu()
{
	while (true) {
		m_mainView.showSampleSubMenu();
		int choice = m_mainView.getSampleSubMenuChoice();
		if (choice == 0) break;
		switch (choice) {
		case 1: {
			std::string id      = m_sampleView.inputId();
			std::string name    = m_sampleView.inputName();
			double prodTime     = m_sampleView.inputAvgProdTime();
			double yield        = m_sampleView.inputYield();
			int stock           = m_sampleView.inputStock();
			try {
				m_sampleCtrl.registerSample(id, name, prodTime, yield, stock);
				m_sampleView.showRegisterSuccess(id);
			} catch (const std::exception& e) {
				m_mainView.showError(e.what());
			}
			break;
		}
		case 2:
			m_sampleView.showSampleList(m_sampleCtrl.getAllSamples());
			break;
		case 3: {
			std::string keyword = m_sampleView.inputSearchKeyword();
			m_sampleView.showSampleList(m_sampleCtrl.searchByName(keyword));
			break;
		}
		default:
			m_mainView.showError("없는 메뉴입니다.");
			break;
		}
	}
}

void Application::handleOrderReserve()
{
	std::string sampleId     = m_orderView.inputSampleId();
	std::string customerName = m_orderView.inputCustomerName();
	int quantity             = m_orderView.inputQuantity();
	std::string orderId      = m_orderCtrl.reserveOrder(sampleId, customerName, quantity);
	m_orderView.showReserveSuccess(orderId);
}

void Application::handleOrderApproveReject(long long nowSec)
{
	std::vector<Order> reserved = m_orderCtrl.getReservedOrders();
	m_orderView.showOrderList(reserved);
	if (reserved.empty()) return;

	std::string orderId = m_orderView.inputOrderId();
	m_orderView.showApproveRejectMenu();
	int choice = m_orderView.inputApproveRejectChoice();

	switch (choice) {
	case 1:
		m_orderCtrl.approveOrder(orderId, nowSec);
		m_orderView.showApproveSuccess(orderId);
		break;
	case 2:
		m_orderCtrl.rejectOrder(orderId);
		m_orderView.showRejectSuccess(orderId);
		break;
	default:
		m_mainView.showError("없는 메뉴입니다.");
		break;
	}
}

void Application::handleProduction(long long nowSec)
{
	if (m_prodCtrl.hasJob()) {
		m_prodView.showCurrentJob(m_prodCtrl.peekNextJob(), nowSec);
	}
	m_prodView.showProductionQueue(m_prodCtrl.getQueue(), nowSec);
}

void Application::handleMonitoring(long long nowSec)
{
	m_monView.showOrderSummary(m_monCtrl.getOrderSummary());
	m_monView.showActiveOrders(m_monCtrl.getActiveOrders());
	m_monView.showProductionProgress(m_monCtrl.getProductionProgress(nowSec));
	for (const auto& s : m_sampleCtrl.getAllSamples()) {
		m_monView.showStockStatus(s, m_monCtrl.getStockStatus(s.getId()));
	}
}

void Application::handleRelease()
{
	std::vector<Order> confirmed = m_releaseCtrl.getConfirmedOrders();
	m_orderView.showOrderList(confirmed);
	if (confirmed.empty()) return;

	std::string orderId = m_orderView.inputOrderId();
	m_releaseCtrl.releaseOrder(orderId);
	m_orderView.showReleaseSuccess(orderId);
}