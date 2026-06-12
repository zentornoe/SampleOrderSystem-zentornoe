#pragma once
#include <queue>
#include "../model/ProductionJob.h"
#include "../repository/JsonSampleRepository.h"
#include "../repository/JsonOrderRepository.h"
#include "../repository/JsonProductionJobRepository.h"
#include "../controller/SampleController.h"
#include "../controller/OrderController.h"
#include "../controller/ProductionController.h"
#include "../controller/MonitoringController.h"
#include "../controller/ReleaseController.h"
#include "../view/MainView.h"
#include "../view/SampleView.h"
#include "../view/OrderView.h"
#include "../view/ProductionView.h"
#include "../view/MonitorView.h"

class Application {
public:
	Application();
	void run();

private:
	// 1. Repository (컨트롤러보다 먼저 초기화되어야 한다)
	JsonSampleRepository        m_sampleRepo;
	JsonOrderRepository         m_orderRepo;
	JsonProductionJobRepository m_jobRepo;

	// 2. 공유 생산 큐 (m_jobRepo 초기화 후 load)
	std::queue<ProductionJob> m_prodQueue;

	// 3. Controller (repo/queue 참조를 저장하므로 선언 순서 유지)
	SampleController     m_sampleCtrl;
	OrderController      m_orderCtrl;
	ProductionController m_prodCtrl;
	MonitoringController m_monCtrl;
	ReleaseController    m_releaseCtrl;

	// 4. View
	MainView       m_mainView;
	SampleView     m_sampleView;
	OrderView      m_orderView;
	ProductionView m_prodView;
	MonitorView    m_monView;

	void handleSampleMenu();
	void handleOrderReserve();
	void handleOrderApproveReject(long long nowSec);
	void handleProduction(long long nowSec);
	void handleMonitoring(long long nowSec);
	void handleRelease();
};