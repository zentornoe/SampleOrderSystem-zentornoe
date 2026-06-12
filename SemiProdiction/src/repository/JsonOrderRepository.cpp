#include "JsonOrderRepository.h"
#include "JsonHelper.h"
#include <sstream>
#include <stdexcept>

namespace {
	std::string toJson(const Order& o)
	{
		std::ostringstream oss;
		oss << "{"
			<< "\"orderId\":\"" << o.getOrderId() << "\","
			<< "\"sampleId\":\"" << o.getSampleId() << "\","
			<< "\"customerName\":\"" << o.getCustomerName() << "\","
			<< "\"quantity\":" << o.getQuantity() << ","
			<< "\"status\":\"" << toString(o.getStatus()) << "\","
			<< "\"createdAt\":" << o.getCreatedAt()
			<< "}";
		return oss.str();
	}

	Order fromJson(const std::string& obj)
	{
		return Order(
			jsonGetStr(obj, "orderId"),
			jsonGetStr(obj, "sampleId"),
			jsonGetStr(obj, "customerName"),
			jsonGetInt(obj, "quantity"),
			fromString(jsonGetStr(obj, "status")),
			jsonGetLong(obj, "createdAt")
		);
	}
}

JsonOrderRepository::JsonOrderRepository(const std::string& filePath)
	: m_filePath(filePath)
{
}

void JsonOrderRepository::save(const Order& order)
{
	auto all = load();
	all.push_back(order);
	persist(all);
}

Order JsonOrderRepository::findById(const std::string& id)
{
	for (const auto& o : load())
		if (o.getOrderId() == id) return o;
	throw std::runtime_error("주문을 찾을 수 없습니다: " + id);
}

std::vector<Order> JsonOrderRepository::findAll()
{
	return load();
}

std::vector<Order> JsonOrderRepository::findByStatus(OrderStatus status)
{
	std::vector<Order> result;
	for (const auto& o : load())
		if (o.getStatus() == status) result.push_back(o);
	return result;
}

void JsonOrderRepository::update(const Order& order)
{
	auto all = load();
	for (auto& o : all) {
		if (o.getOrderId() == order.getOrderId()) {
			o = order;
			persist(all);
			return;
		}
	}
	throw std::runtime_error("주문을 찾을 수 없습니다: " + order.getOrderId());
}

bool JsonOrderRepository::exists(const std::string& id)
{
	for (const auto& o : load())
		if (o.getOrderId() == id) return true;
	return false;
}

int JsonOrderRepository::getNextSequence()
{
	return static_cast<int>(load().size()) + 1;
}

void JsonOrderRepository::persist(const std::vector<Order>& orders)
{
	jsonPersistArray(m_filePath, orders, toJson);
}

std::vector<Order> JsonOrderRepository::load() const
{
	std::vector<Order> result;
	for (const auto& obj : jsonSplitObjects(jsonReadFile(m_filePath)))
		result.push_back(fromJson(obj));
	return result;
}
