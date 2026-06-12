# Phase 4 상세 구현 계획 — 주문 접수 (OrderController::reserveOrder)

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_03.md](./phase_03.md)
> 대상 Phase: 4 (OrderController::reserveOrder)
> 총 테스트: 5개

---

## 개요

Phase 4는 고객이 시료를 주문하면 `RESERVED` 상태로 주문을 생성하는 접수 기능을 구현한다.

**MVC 분리 원칙**
- `OrderController` — 비즈니스 로직 (시료 존재 확인, 주문번호 채번, Repository 위임). `std::cout` 사용 금지.
- `OrderView` — 콘솔 출력만 담당. 이 Phase에서 구현하고, 단독 테스트는 Phase 5에서 진행.

**Phase 3과의 공통 패턴**
- Controller가 직접 검증하는 규칙: 시료 존재 여부 1가지
- 나머지 유효성 검사(수량·고객명): `Order` 생성자가 `invalid_argument`로 처리 → 컨트롤러는 그대로 전파

---

## 변경·신규 파일 목록

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/controller/OrderController.h` | 수정 | `getReservedOrders` 메서드 선언 추가 (Phase 5 사전 준비) |
| `src/controller/OrderController.cpp` | 신규 | `reserveOrder` 전체 구현, 나머지 메서드는 스텁 |
| `src/view/OrderView.cpp` | 신규 | 전체 구현 (단독 테스트는 Phase 5에서 진행) |
| `test/test_order_controller.cpp` | 신규 | `reserveOrder` 테스트 5개 |
| `SemiProdiction.vcxproj` | 수정 | 위 `.cpp` 3개 추가 |

---

## 헤더 변경 — OrderController.h

현재 스텁에 Phase 5에서 필요한 `getReservedOrders`가 누락되어 있다. 지금 추가해 두어야 Phase 5 테스트 파일 작성 시 컴파일 에러가 없다.

```
추가할 선언:
  std::vector<Order> getReservedOrders() const;
```

변경 후 전체 public 메서드:

| 메서드 | Phase | 설명 |
|--------|-------|------|
| `reserveOrder(sampleId, customerName, quantity)` | 4 | 주문 접수 |
| `getReservedOrders() const` | 4 선언 / 5 테스트 | RESERVED 목록 조회 |
| `getOrder(orderId) const` | 4 구현 (스텁 수준) | 단일 주문 조회 |
| `getAllOrders() const` | 4 구현 (스텁 수준) | 전체 주문 조회 |
| `approveOrder(orderId)` | 5 | 주문 승인 |
| `rejectOrder(orderId)` | 5 | 주문 거절 |

---

## OrderController 설계

### 생성자

```
OrderController(IOrderRepository& orderRepo,
                ISampleRepository& sampleRepo,
                std::queue<ProductionJob>& prodQueue)
  m_orderRepo  = orderRepo
  m_sampleRepo = sampleRepo
  m_prodQueue  = prodQueue
```

---

### reserveOrder(sampleId, customerName, quantity) → string

**구현 순서**

```
1. m_sampleRepo.exists(sampleId) == false
     → throw runtime_error("존재하지 않는 시료 ID: " + sampleId)

2. int seq = m_orderRepo.getNextSequence()

3. std::string orderId = Order::generateOrderId(seq)

4. Order order(orderId, sampleId, customerName, quantity)
     └ quantity <= 0   → Order 생성자가 invalid_argument 발생 (자동 전파)
     └ customerName 빈 문자열 → Order 생성자가 invalid_argument 발생 (자동 전파)
     └ 생성 시 m_status = RESERVED (Order 생성자 기본값)

5. m_orderRepo.save(order)

6. return orderId
```

**책임 분리 기준**

| 검증 항목 | 담당 |
|----------|------|
| 시료 ID 존재 여부 | OrderController (sampleRepo.exists 호출) |
| 수량 ≥ 1 | Order 생성자 → `invalid_argument` 자동 전파 |
| 고객명 비어있지 않음 | Order 생성자 → `invalid_argument` 자동 전파 |

---

### getReservedOrders() const → vector\<Order\>

```
return m_orderRepo.findByStatus(OrderStatus::RESERVED)
```

---

### getOrder(orderId) const → Order

```
return m_orderRepo.findById(orderId)
  └ 없으면 Repository가 runtime_error 발생 (그대로 전파)
```

---

### getAllOrders() const → vector\<Order\>

```
return m_orderRepo.findAll()
```

---

### approveOrder(orderId) — Phase 5 구현 스텁

```
throw std::logic_error("approveOrder: 미구현 (Phase 5)")
```

Phase 5 설계 문서에서 전체 구현.

---

### rejectOrder(orderId) — Phase 5 구현 스텁

```
throw std::logic_error("rejectOrder: 미구현 (Phase 5)")
```

---

## OrderView 설계

### 메서드별 출력 규칙

모든 메서드는 `const`, `std::cout` 사용, 비즈니스 로직 없음.

#### `showOrderList(const vector<Order>& orders) const`

```
orders가 비어있으면:
  "주문이 없습니다." 출력

비어있지 않으면:
  헤더 행: "주문번호 | 시료 ID | 고객명 | 수량 | 상태"
  각 주문: orderId, sampleId, customerName, quantity, toString(status) 출력
```

#### `showOrder(const Order& order) const`

```
단일 주문 상세 정보 출력
orderId, sampleId, customerName, quantity, status, createdAt 포함
```

#### `showReserveSuccess(const string& orderId) const`

```
"[주문 접수 완료] " + orderId 형식으로 출력
```

#### `showApproveSuccess(const string& orderId) const`

```
"[주문 승인 완료] " + orderId 형식으로 출력
```

#### `showRejectSuccess(const string& orderId) const`

```
"[주문 거절 완료] " + orderId 형식으로 출력
```

#### 입력 메서드

| 메서드 | 동작 |
|--------|------|
| `inputSampleId()` | "시료 ID: " 출력 후 `std::cin >> value` |
| `inputCustomerName()` | "고객명: " 출력 후 `std::getline(std::cin >> std::ws, value)` |
| `inputQuantity()` | "수량: " 출력 후 `std::cin >> value` |
| `inputOrderId()` | "주문번호: " 출력 후 `std::cin >> value` |

> `inputCustomerName`은 공백 포함 이름을 허용하기 위해 `getline` 사용.
> `std::ws`로 앞선 개행 문자를 소비해야 정상 동작한다.

---

## 테스트 설계 (5개)

**파일**: `test/test_order_controller.cpp`

**공통 준비 코드**

```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/controller/OrderController.h"
#include "mock/MockOrderRepository.h"
#include "mock/MockSampleRepository.h"

class OrderControllerTest : public ::testing::Test {
protected:
    MockOrderRepository  mockOrderRepo;
    MockSampleRepository mockSampleRepo;
    std::queue<ProductionJob> prodQueue;
    OrderController ctrl{ mockOrderRepo, mockSampleRepo, prodQueue };
};
```

---

### 테스트 1 — 정상 예약 시 RESERVED 상태로 save가 1회 호출되는지

```
GIVEN: sampleRepo.exists("S-001") → true
       orderRepo.getNextSequence() → 1
WHEN:  ctrl.reserveOrder("S-001", "삼성전자", 200)
THEN:  orderRepo.save() 가 정확히 1회 호출됨
       save에 전달된 Order의 getStatus() == RESERVED
```

GMock 설정:

```cpp
EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));

Order capturedOrder("dummy", "S-001", "삼성전자", 200);
EXPECT_CALL(mockOrderRepo, save(testing::_))
    .WillOnce(testing::SaveArg<0>(&capturedOrder));

ctrl.reserveOrder("S-001", "삼성전자", 200);

EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::RESERVED);
```

---

### 테스트 2 — 반환된 주문번호 형식이 ORD-YYYYMMDD-NNNN인지

```
GIVEN: sampleRepo.exists("S-001") → true
       orderRepo.getNextSequence() → 1
WHEN:  orderId = ctrl.reserveOrder("S-001", "삼성전자", 200)
THEN:  orderId.substr(0, 4) == "ORD-"
       orderId.substr(orderId.length() - 4) == "0001"
       orderId.length() == 17   // "ORD-" + 8자리 날짜 + "-" + 4자리 시퀀스
```

GMock 설정:

```cpp
EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
EXPECT_CALL(mockOrderRepo, save(testing::_)).Times(1);

auto orderId = ctrl.reserveOrder("S-001", "삼성전자", 200);

EXPECT_EQ(orderId.substr(0, 4), "ORD-");
EXPECT_EQ(orderId.substr(orderId.length() - 4), "0001");
EXPECT_EQ(static_cast<int>(orderId.length()), 17);
```

---

### 테스트 3 — 존재하지 않는 시료 ID로 주문 시 runtime_error 발생

```
GIVEN: sampleRepo.exists("X-999") → false
WHEN:  ctrl.reserveOrder("X-999", "삼성전자", 200)
THEN:  std::runtime_error 발생
       orderRepo.getNextSequence() 는 호출되지 않음
       orderRepo.save() 는 호출되지 않음
```

GMock 설정:

```cpp
EXPECT_CALL(mockSampleRepo, exists("X-999")).WillOnce(Return(false));
EXPECT_CALL(mockOrderRepo, getNextSequence()).Times(0);
EXPECT_CALL(mockOrderRepo, save(testing::_)).Times(0);

EXPECT_THROW(
    ctrl.reserveOrder("X-999", "삼성전자", 200),
    std::runtime_error
);
```

---

### 테스트 4 — 수량이 0 이하이면 invalid_argument 발생

```
GIVEN: sampleRepo.exists("S-001") → true
       orderRepo.getNextSequence() → 1
       quantity = 0
WHEN:  ctrl.reserveOrder("S-001", "삼성전자", 0)
THEN:  std::invalid_argument 발생 (Order 생성자에서 전파)
       orderRepo.save() 는 호출되지 않음
```

GMock 설정:

```cpp
EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
EXPECT_CALL(mockOrderRepo, save(testing::_)).Times(0);

EXPECT_THROW(
    ctrl.reserveOrder("S-001", "삼성전자", 0),
    std::invalid_argument
);
```

경계값 추가 검증: `quantity = -1`도 동일한 예외 발생 확인.

---

### 테스트 5 — 고객명이 비어있으면 invalid_argument 발생

```
GIVEN: sampleRepo.exists("S-001") → true
       orderRepo.getNextSequence() → 1
       customerName = ""
WHEN:  ctrl.reserveOrder("S-001", "", 200)
THEN:  std::invalid_argument 발생 (Order 생성자에서 전파)
       orderRepo.save() 는 호출되지 않음
```

GMock 설정:

```cpp
EXPECT_CALL(mockSampleRepo, exists("S-001")).WillOnce(Return(true));
EXPECT_CALL(mockOrderRepo, getNextSequence()).WillOnce(Return(1));
EXPECT_CALL(mockOrderRepo, save(testing::_)).Times(0);

EXPECT_THROW(
    ctrl.reserveOrder("S-001", "", 200),
    std::invalid_argument
);
```

---

## vcxproj 추가 항목

### 항상 컴파일 (Debug + Release 모두)

```xml
<ClCompile Include="src\controller\OrderController.cpp" />
<ClCompile Include="src\view\OrderView.cpp" />
```

### Debug 전용 테스트

```xml
<ClCompile Include="test\test_order_controller.cpp"
           Condition="'$(Configuration)'=='Debug'" />
```

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 누적 46개 통과 (Phase 0~3의 41개 + Phase 4의 5개) |
| MVC 분리 | OrderController에 `std::cout` 없음, OrderView에 비즈니스 로직 없음 |
| Mock 활용 | 테스트 전부 Mock 사용, 파일 I/O 없음 |
| 스텁 안전성 | approveOrder / rejectOrder 호출 시 `logic_error` 발생 (묵묵히 성공하는 스텁 금지) |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| `getReservedOrders` 선언 추가 | `OrderController.h`에 지금 추가해야 Phase 5 설계 시 헤더 재수정 불필요 |
| 유효성 검증 위치 | 시료 존재 여부만 Controller 책임. 수량·고객명은 Order 생성자에 위임 (Phase 3 패턴과 동일) |
| `getNextSequence` 호출 순서 | exists 확인 실패 시 `getNextSequence`를 호출하면 안 됨. 순서를 지켜야 테스트 3이 통과됨 |
| `SaveArg<0>` 사용 | 테스트 1에서 save에 전달된 Order 객체의 status를 검증하기 위해 `SaveArg` 사용. `capturedOrder`를 미리 유효한 값으로 초기화해야 컴파일 오류 없음 |
| `inputCustomerName`의 `getline` | `std::cin >>` 이후 `getline` 호출 시 버퍼에 남은 개행 문자 처리를 위해 `std::ws` 필수 |
| approveOrder / rejectOrder 스텁 | 빈 함수나 `return` 스텁은 Phase 5에서 실수로 통과될 수 있음. `logic_error`로 명시적 미구현 표시 |
