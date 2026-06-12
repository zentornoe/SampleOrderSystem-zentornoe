#pragma once
#include <string>

enum class StockStatus {
	SUFFICIENT,
	SHORTAGE,
	DEPLETED
};

struct OrderSummary {
	int totalReserved = 0;
	int totalConfirmed = 0;
	int totalProducing = 0;
	int totalRelease = 0;
};

struct ProductionProgress {
	bool        hasJob          = false;
	std::string orderId;
	std::string sampleId;
	int         currentProd     = 0;
	int         totalProd       = 0;
	long long   completionTimeSec = 0;   // Unix timestamp
};
