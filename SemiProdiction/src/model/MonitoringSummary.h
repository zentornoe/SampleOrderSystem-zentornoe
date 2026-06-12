#pragma once

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
