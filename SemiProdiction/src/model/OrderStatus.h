#pragma once
#include <string>
#include <stdexcept>

enum class OrderStatus {
	RESERVED,
	CONFIRMED,
	PRODUCING,
	RELEASE,
	REJECTED
};

std::string toString(OrderStatus status);
OrderStatus fromString(const std::string& s);
