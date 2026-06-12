#include "OrderStatus.h"

std::string toString(OrderStatus status)
{
	switch (status) {
	case OrderStatus::RESERVED:  return "RESERVED";
	case OrderStatus::CONFIRMED: return "CONFIRMED";
	case OrderStatus::PRODUCING: return "PRODUCING";
	case OrderStatus::RELEASE:   return "RELEASE";
	case OrderStatus::REJECTED:  return "REJECTED";
	default:                     return "UNKNOWN";
	}
}

OrderStatus fromString(const std::string& s)
{
	if (s == "RESERVED")  return OrderStatus::RESERVED;
	if (s == "CONFIRMED") return OrderStatus::CONFIRMED;
	if (s == "PRODUCING") return OrderStatus::PRODUCING;
	if (s == "RELEASE")   return OrderStatus::RELEASE;
	if (s == "REJECTED")  return OrderStatus::REJECTED;
	throw std::invalid_argument("알 수 없는 상태: " + s);
}
