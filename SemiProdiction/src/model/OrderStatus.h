#pragma once
#include <string>

enum class OrderStatus {
	RESERVED,
	CONFIRMED,
	PRODUCING,
	RELEASE,
	REJECTED
};

std::string toString(OrderStatus status);
OrderStatus fromString(const std::string& s);
