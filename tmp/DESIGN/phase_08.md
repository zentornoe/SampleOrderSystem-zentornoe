# Phase 8 상세 구현 계획 — 출고 처리 (ReleaseController)

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_07.md](./phase_07.md)
> 대상 Phase: 8 (ReleaseController)
> 총 테스트: 5개 (test_release_controller.cpp 신규)

---

## 개요

Phase 8은 `CONFIRMED` 상태 주문을 출고하여 `RELEASE`로 전환하는 **출고 처리 기능**을 구현한다.
출고 시 주문 상태 전이와 함께 실재고 및 예약 재고가 동시에 차감된다.

**상태 전이**

```
CONFIRMED → release() → RELEASE
```

**재고 변화**

```
출고 전: stock=300, reservedQty=150
출고 후: stock=150, reservedQty=0
```

PRD: "출고 시 예약 수량 및 재고에서 출고 수량만큼 빠짐"

---

## 기존 파일 현황

| 파일 | 상태 | 내용 |
|------|------|------|
| `src/controller/ReleaseController.h` | 기존 존재 | `releaseOrder(orderId)` 만 선언. `getConfirmedOrders()` 없음 |
| `src/controller/ReleaseController.cpp` | **없음** | 신규 작성 필요 |
| `test/test_release_controller.cpp` | **없음** | 신규 작성 필요 |

---

## 변경·신규 파일 목록

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/controller/ReleaseController.h` | **수정** | `getConfirmedOrders()` 선언 추가 |
| `src/controller/ReleaseController.cpp` | 신규 | 전체 구현 |
| `test/test_release_controller.cpp` | 신규 | 테스트 5개 |
| `SemiProdiction.vcxproj` | 수정 | 위 .cpp 2개 추가 |

---

## ReleaseController.h 변경

`getConfirmedOrders()` 선언을 추가한다. `Order.h` include도 필요하다.

### 변경 후 전체 public 인터페이스

```cpp
#pragma once
#include <vector>
#include <string>
#include "../model/Order.h"
#include "../repository/IOrderRepository.h"
#include "../repository/ISampleRepository.h"

class ReleaseController {
public:
    explicit ReleaseController(IOrderRepository& orderRepo,
                               ISampleRepository& sampleRepo);

    std::vector<Order> getConfirmedOrders() const;
    void releaseOrder(const std::string& orderId);

private:
    IOrderRepository& m_orderRepo;
    ISampleRepository& m_sampleRepo;
};
```

---

## ReleaseController.cpp 구현 설계

### 생성자

```cpp
ReleaseController::ReleaseController(IOrderRepository& orderRepo,
                                     ISampleRepository& sampleRepo)
    : m_orderRepo(orderRepo)
    , m_sampleRepo(sampleRepo)
{}
```

---

### getConfirmedOrders() const

```
return m_orderRepo.findByStatus(OrderStatus::CONFIRMED)
```

`findByStatus`에 위임하므로 별도 필터링 로직 없음.

---

### releaseOrder(orderId)

```
1. Order order = m_orderRepo.findById(orderId)

2. if (order.getStatus() != OrderStatus::CONFIRMED)
       throw std::logic_error("출고 가능한 주문이 아닙니다: " + orderId)

3. Sample sample = m_sampleRepo.findById(order.getSampleId())

4. // 출고 수량이 실재고를 초과하면 처리 불가
   if (sample.getStock() < order.getQuantity())
       throw std::runtime_error("재고 부족으로 출고할 수 없습니다: " + orderId)

5. order.release()                              // CONFIRMED → RELEASE
   m_orderRepo.update(order)

6. sample.reduceStock(order.getQuantity())      // 실재고 차감
   // 승인 시 예약한 수량을 해제 (직접 CONFIRMED 경로)
   if (sample.getReservedQty() >= order.getQuantity())
       sample.releaseQty(order.getQuantity())
   m_sampleRepo.update(sample)
```

**step 4 판단 기준**: `sample.getStock()`(실재고) 기준으로 확인.
`getAvailableStock()`(가용재고)이 아닌 이유 — 출고는 이미 예약된 재고를 실제로 내보내는 행위이므로
실재고가 주문 수량 이상이면 출고 가능하다.

**step 6 `releaseQty` 조건부 호출**: 재고 충분 경로(직접 CONFIRMED)에서는 `approveOrder` 시
`sample.reserveQty(qty)` 가 호출되어 reservedQty가 설정된다. 생산 완료 경로(PRODUCING → CONFIRMED)에서는
`reserveQty`가 호출되지 않아 reservedQty가 0이므로 조건으로 보호한다.

---

## 테스트 설계 (5개)

**파일**: `test/test_release_controller.cpp` (신규)

**의존성**: MockOrderRepository, MockSampleRepository (기존 파일 재사용)

**공통 Fixture**

```cpp
namespace {
    const std::string TEST_ORDER_ID  = "ORD-20260612-0001";
    const std::string TEST_SAMPLE_ID = "S-001";
}

class ReleaseControllerTest : public ::testing::Test {
protected:
    MockOrderRepository  mockOrderRepo;
    MockSampleRepository mockSampleRepo;
    std::unique_ptr<ReleaseController> ctrl;

    void SetUp() override {
        ctrl = std::make_unique<ReleaseController>(mockOrderRepo, mockSampleRepo);
    }

    // CONFIRMED 상태 주문 헬퍼
    Order makeConfirmedOrder(int quantity = 150) {
        return Order(TEST_ORDER_ID, TEST_SAMPLE_ID, "삼성전자", quantity,
                     OrderStatus::CONFIRMED, 0LL);
    }

    // 재고와 예약량을 설정한 시료 헬퍼
    Sample makeSample(int stock, int reservedQty = 0) {
        return Sample(TEST_SAMPLE_ID, "시료A", 5.0, 0.92, stock, reservedQty);
    }
};
```

---

### 테스트 1 — 출고 후 주문 상태가 RELEASE로 전환됨

```
GIVEN: CONFIRMED 주문, 재고 300 / 예약 150
WHEN:  releaseOrder(orderId)
THEN:  orderRepo.update 에 전달된 order.getStatus() == RELEASE
MOCK:  findById(orderId)    → CONFIRMED 주문
       findById(sampleId)   → Sample(stock=300, reservedQty=150)
       orderRepo.update(_)  → SaveArg 로 캡처
       sampleRepo.update(_) → Times(1)
```

---

### 테스트 2 — 출고 후 재고가 정확히 차감됨 (300 → 150)

```
GIVEN: 재고 300, 주문 수량 150
WHEN:  releaseOrder(orderId)
THEN:  sampleRepo.update 에 전달된 sample.getStock() == 150
MOCK:  findById(orderId)    → CONFIRMED 주문 (quantity=150)
       findById(sampleId)   → Sample(stock=300, reservedQty=150)
       orderRepo.update(_)  → Times(1)
       sampleRepo.update(_) → SaveArg 로 캡처
```

---

### 테스트 3 — CONFIRMED 아닌 주문 출고 시 logic_error

```
GIVEN: PRODUCING 상태 주문
WHEN:  releaseOrder(orderId)
THEN:  std::logic_error 발생
       sampleRepo.findById 호출 0회
       orderRepo.update 호출 0회
MOCK:  findById(orderId) → PRODUCING 주문
```

---

### 테스트 4 — getConfirmedOrders: CONFIRMED 목록만 반환

```
GIVEN: findByStatus(CONFIRMED) 호출 시 2건 반환되도록 설정
WHEN:  getConfirmedOrders()
THEN:  result.size() == 2
       findByStatus 에 CONFIRMED 가 전달됨
MOCK:  findByStatus(OrderStatus::CONFIRMED) → CONFIRMED 주문 2건 반환
```

---

### 테스트 5 — 재고 부족 시 runtime_error

```
GIVEN: 재고 50, 주문 수량 150
WHEN:  releaseOrder(orderId)
THEN:  std::runtime_error 발생
       orderRepo.update  호출 0회
       sampleRepo.update 호출 0회
MOCK:  findById(orderId)  → CONFIRMED 주문 (quantity=150)
       findById(sampleId) → Sample(stock=50)
```

---

## vcxproj 추가 항목

### 항상 컴파일 (Debug + Release) — Controller 블록에 추가

```xml
<ClCompile Include="src\controller\ReleaseController.cpp" />
```

### Debug 전용 테스트

```xml
<ClCompile Include="test\test_release_controller.cpp"
           Condition="'$(Configuration)'=='Debug'" />
```

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 누적 81개 통과 (기존 76개 + Phase 8의 5개) |
| 상태 전이 | CONFIRMED → RELEASE, 非CONFIRMED 시 logic_error |
| 재고 차감 | `reduceStock(qty)` 호출로 실재고 정확히 감소 |
| 예약 해제 | reservedQty >= qty 일 때만 `releaseQty(qty)` 호출 |
| 재고 부족 | stock < qty 시 runtime_error (update 미호출) |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| `getConfirmedOrders` 추가 | ReleaseController.h 헤더 수정 필요. `findByStatus(CONFIRMED)` 위임 |
| `releaseQty` 조건부 호출 | 직접 CONFIRMED 경로는 reservedQty가 설정됨. 생산 완료 CONFIRMED 경로는 reservedQty=0이므로 조건 보호 필수 |
| 재고 검증 기준 | `getStock()` (실재고) 기준. 이미 예약된 물량을 실제로 출고하는 것이므로 가용재고가 아닌 실재고로 판단 |
| 예외 타입 구분 | 상태 오류 → `logic_error`, 재고 부족 → `runtime_error` (기존 Controller 패턴 통일) |