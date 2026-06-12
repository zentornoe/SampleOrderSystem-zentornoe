#pragma once
#include <string>
#include <ctime>
#include "OrderStatus.h"

class Order {
public:
	Order(const std::string& orderId, const std::string& sampleId,
		  const std::string& customerName, int quantity);
	// 역직렬화용 생성자 — 파일 복원 시 사용, 유효성 검사 없음
	Order(const std::string& orderId, const std::string& sampleId,
		  const std::string& customerName, int quantity,
		  OrderStatus status, long long createdAt);

	const std::string& getOrderId() const;
	const std::string& getSampleId() const;
	const std::string& getCustomerName() const;
	int getQuantity() const;
	OrderStatus getStatus() const;
	long long getCreatedAt() const;

	// 상태 전이 메서드 — 허용되지 않는 전이 시 std::logic_error 발생
	void confirm();				// RESERVED  → CONFIRMED  (재고 충분)
	void sendToProduction();	// RESERVED  → PRODUCING  (재고 부족)
	void completeProduction();	// PRODUCING → CONFIRMED  (생산 완료)
	void release();				// CONFIRMED → RELEASE
	void reject();				// RESERVED  → REJECTED

	bool isMonitored() const;
	static std::string generateOrderId(int sequence);

private:
	Order() = default;				// Repository 역직렬화 전용

	std::string m_orderId;
	std::string m_sampleId;
	std::string m_customerName;
	int m_quantity = 0;
	OrderStatus m_status = OrderStatus::RESERVED;
	long long m_createdAt = 0;
};
