# Phase 5 상세 구현 계획 — 주문 승인/거절 (OrderController::approveOrder / rejectOrder)

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_04.md](./phase_04.md)
> 대상 Phase: 5 (OrderController::approveOrder / rejectOrder)
> 총 테스트: 7개 (test_order_controller.cpp 에 추가)

---

## 개요

Phase 5는 RESERVED 상태 주문을 **승인(approve)** 하거나 **거절(reject)** 하는 기능을 구현한다.

**승인 시 재고 분기**

```
승인 → sample.isStockEnough(qty) == true   → order.confirm()          → CONFIRMED
     → sample.isStockEnough(qty) == false  → order.sendToProduction() → PRODUCING
                                              + ProductionJob 를 prodQueue 에 push
거절 → order.reject()                      → REJECTED
```

**MVC 분리 원칙**
- `OrderController` — 재고 판단, 상태 전이 호출, Repository 위임. `std::cout` 사용 금지.
- `OrderView` — Phase 4에서 이미 구현됨. 이 Phase에서는 건드리지 않는다.

**이미 구현된 항목** (Phase 4에서 완료)
- `OrderController.h` — `approveOrder`, `rejectOrder`, `getReservedOrders` 선언 존재
- `OrderController.cpp` — `approveOrder`, `rejectOrder` 는 `logic_error` 스텁 상태
- `test/test_order_controller.cpp` — 기존 파일에 테스트 7개 추가

---

## 변경·신규 파일 목록

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/controller/OrderController.cpp` | 수정 | `approveOrder`, `rejectOrder` 스텁 → 실제 구현 |
| `test/test_order_controller.cpp` | 수정 | 기존 파일 하단에 테스트 7개 추가 |
| `SemiProdiction.vcxproj` | 변경 없음 | 이미 두 파일 모두 등록되어 있음 |

---

## OrderController 구현 설계

### approveOrder(orderId)

```
1. Order order = m_orderRepo.findById(orderId)
     └ 없으면 Repository 가 runtime_error 발생 (그대로 전파)

2. if (order.getStatus() != OrderStatus::RESERVED)
     throw std::logic_error("승인 가능한 주문이 아닙니다: " + orderId)
     └ sampleRepo 조회 전에 실패 → findById(sampleId) 미호출

3. Sample sample = m_sampleRepo.findById(order.getSampleId())
     └ 없으면 runtime_error 전파

4-A. 재고 충분 — sample.isStockEnough(order.getQuantity()) == true
     order.confirm()
     sample.reserveQty(order.getQuantity())   // 가용재고 감소
     m_sampleRepo.update(sample)
     m_orderRepo.update(order)

4-B. 재고 부족 — isStockEnough == false
     int shortage = order.getQuantity() - sample.getStock()
     ProductionJob job(order.getOrderId(), order.getSampleId(),
                       shortage, sample.getYield(), sample.getAvgProdTime())
     m_prodQueue.push(job)
     order.sendToProduction()
     m_orderRepo.update(order)
     // sampleRepo.update 호출 안 함 — 재고 변경은 생산 완료 시 ProductionController 가 처리
```

**재고 판단 기준** (CLAUDE.md 명세 그대로)

| 조건 | 분기 |
|------|------|
| `stock >= qty` (여유 포함 경계) | CONFIRMED |
| `0 < stock < qty` (부족) | PRODUCING |
| `stock == 0` (고갈) | PRODUCING (`shortage == qty`) |

---

### rejectOrder(orderId)

```
1. Order order = m_orderRepo.findById(orderId)

2. order.reject()
     └ RESERVED 이외 상태에서 호출 시 Order 생성자/메서드가 logic_error 발생
       → 그대로 전파

3. m_orderRepo.update(order)
```

---

### getReservedOrders() const — 이미 구현됨 (Phase 4)

```
return m_orderRepo.findByStatus(OrderStatus::RESERVED)
```

---

## 생산량 공식 확인

```
shortage   = orderQty - currentStock
actualProd = (int)ceil(shortage / (yield * 0.9))
totalTime  = avgProdTime * actualProd
```

| 입력 | 계산 | 결과 |
|------|------|------|
| qty=200, stock=30, yield=0.92 | ceil(170 / 0.828) = ceil(205.31) | **206** |

`ProductionJob` 생성자가 이 공식을 내부 적용하므로 Controller는 `shortage` 만 계산해서 전달한다.

---

## 테스트 설계 (7개)

**파일**: `test/test_order_controller.cpp` (기존 Fixture `OrderControllerTest` 재사용)

**공통 헬퍼 상수**

```cpp
// 테스트 파일 상단 익명 namespace 에 선언
namespace {
    const std::string SAMPLE_ID = "S-001";
    const std::string ORDER_ID  = "ORD-20260612-0001";
}
```

---

### 테스트 6 — 재고 충분 시 CONFIRMED 전환

```
GIVEN: orderRepo.findById(ORDER_ID) → Order(RESERVED, sampleId="S-001", qty=100)
       sampleRepo.findById("S-001") → Sample(stock=200, yield=0.92, avgProdTime=5.0)
WHEN:  ctrl->approveOrder(ORDER_ID)
THEN:  orderRepo.update 에 전달된 Order 의 getStatus() == CONFIRMED
       sampleRepo.update 가 정확히 1회 호출됨
       prodQueue 가 비어 있음
```

GMock 설정:

```cpp
Order  inOrder (ORDER_ID, SAMPLE_ID, "삼성전자", 100);
Sample inSample(SAMPLE_ID, "테스트시료", 5.0, 0.92, 200);
Order  capturedOrder = inOrder;

EXPECT_CALL(mockOrderRepo,  findById(ORDER_ID)).WillOnce(Return(inOrder));
EXPECT_CALL(mockSampleRepo, findById(SAMPLE_ID)).WillOnce(Return(inSample));
EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
EXPECT_CALL(mockSampleRepo, update(_)).Times(1);

ctrl->approveOrder(ORDER_ID);

EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
EXPECT_TRUE(prodQueue.empty());
```

---

### 테스트 7 — 재고 부족 시 PRODUCING 전환 + 생산라인 등록

```
GIVEN: orderRepo.findById → Order(RESERVED, qty=200)
       sampleRepo.findById → Sample(stock=30, yield=0.92, avgProdTime=5.0)
WHEN:  ctrl->approveOrder(ORDER_ID)
THEN:  update 된 Order 의 getStatus() == PRODUCING
       sampleRepo.update 는 호출되지 않음
       prodQueue.size() == 1
```

GMock 설정:

```cpp
Order  inOrder (ORDER_ID, SAMPLE_ID, "삼성전자", 200);
Sample inSample(SAMPLE_ID, "테스트시료", 5.0, 0.92, 30);
Order  capturedOrder = inOrder;

EXPECT_CALL(mockOrderRepo,  findById(ORDER_ID)).WillOnce(Return(inOrder));
EXPECT_CALL(mockSampleRepo, findById(SAMPLE_ID)).WillOnce(Return(inSample));
EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
EXPECT_CALL(mockSampleRepo, update(_)).Times(0);

ctrl->approveOrder(ORDER_ID);

EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::PRODUCING);
EXPECT_EQ(prodQueue.size(), 1u);
```

---

### 테스트 8 — 재고 = 주문 수량 경계값 → CONFIRMED (PRODUCING 이 아님)

```
GIVEN: orderRepo.findById → Order(RESERVED, qty=100)
       sampleRepo.findById → Sample(stock=100)    ← stock == qty (경계)
WHEN:  ctrl->approveOrder(ORDER_ID)
THEN:  update 된 Order 의 getStatus() == CONFIRMED
       prodQueue 가 비어 있음
```

GMock 설정:

```cpp
Order  inOrder (ORDER_ID, SAMPLE_ID, "삼성전자", 100);
Sample inSample(SAMPLE_ID, "테스트시료", 5.0, 0.92, 100);
Order  capturedOrder = inOrder;

EXPECT_CALL(mockOrderRepo,  findById(ORDER_ID)).WillOnce(Return(inOrder));
EXPECT_CALL(mockSampleRepo, findById(SAMPLE_ID)).WillOnce(Return(inSample));
EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
EXPECT_CALL(mockSampleRepo, update(_)).Times(1);

ctrl->approveOrder(ORDER_ID);

EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
EXPECT_TRUE(prodQueue.empty());
```

---

### 테스트 9 — RESERVED 아닌 주문 승인 시 logic_error, sampleRepo 미조회

```
GIVEN: orderRepo.findById → Order(CONFIRMED, qty=100)   ← 이미 승인된 주문
WHEN:  ctrl->approveOrder(ORDER_ID)
THEN:  std::logic_error 발생
       sampleRepo.findById 는 호출되지 않음 (상태 확인 후 즉시 throw)
       orderRepo.update 는 호출되지 않음
```

GMock 설정:

```cpp
Order inOrder(ORDER_ID, SAMPLE_ID, "삼성전자", 100,
              OrderStatus::CONFIRMED, 0);

EXPECT_CALL(mockOrderRepo,  findById(ORDER_ID)).WillOnce(Return(inOrder));
EXPECT_CALL(mockSampleRepo, findById(_)).Times(0);
EXPECT_CALL(mockOrderRepo,  update(_)).Times(0);

EXPECT_THROW(ctrl->approveOrder(ORDER_ID), std::logic_error);
```

> `Order(id, sampleId, customerName, qty, status, createdAt)` 역직렬화 생성자를
> 사용하여 상태가 CONFIRMED 인 주문을 직접 생성한다.

---

### 테스트 10 — 거절 시 REJECTED 전환, orderRepo.update 1회

```
GIVEN: orderRepo.findById → Order(RESERVED, qty=100)
WHEN:  ctrl->rejectOrder(ORDER_ID)
THEN:  update 된 Order 의 getStatus() == REJECTED
       orderRepo.update 가 정확히 1회 호출됨
```

GMock 설정:

```cpp
Order inOrder (ORDER_ID, SAMPLE_ID, "삼성전자", 100);
Order capturedOrder = inOrder;

EXPECT_CALL(mockOrderRepo, findById(ORDER_ID)).WillOnce(Return(inOrder));
EXPECT_CALL(mockOrderRepo, update(_)).WillOnce(SaveArg<0>(&capturedOrder));

ctrl->rejectOrder(ORDER_ID);

EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::REJECTED);
```

---

### 테스트 11 — RESERVED 목록 조회가 Repository 에 올바르게 위임되는지

```
GIVEN: orderRepo.findByStatus(RESERVED) → [Order1, Order2]
WHEN:  ctrl->getReservedOrders()
THEN:  반환된 벡터 크기 == 2
       두 주문 모두 getStatus() == RESERVED
```

GMock 설정:

```cpp
std::vector<Order> reserved = {
    Order(ORDER_ID, SAMPLE_ID, "삼성전자",  100),
    Order("ORD-20260612-0002", SAMPLE_ID, "SK하이닉스", 50)
};

EXPECT_CALL(mockOrderRepo, findByStatus(OrderStatus::RESERVED))
    .WillOnce(Return(reserved));

auto result = ctrl->getReservedOrders();

EXPECT_EQ(result.size(), 2u);
```

---

### 테스트 12 — 생산라인 등록 시 실생산량 공식(206) 검증

```
GIVEN: orderRepo.findById → Order(RESERVED, sampleId="S-001", qty=200)
       sampleRepo.findById → Sample(stock=30, yield=0.92, avgProdTime=5.0)
       shortage = 200 - 30 = 170
       actualProd = ceil(170 / (0.92 * 0.9)) = ceil(205.31) = 206
WHEN:  ctrl->approveOrder(ORDER_ID)
THEN:  prodQueue.front().getActualProd() == 206
       prodQueue.front().getOrderId()   == ORDER_ID
```

GMock 설정:

```cpp
Order  inOrder (ORDER_ID, SAMPLE_ID, "삼성전자", 200);
Sample inSample(SAMPLE_ID, "테스트시료", 5.0, 0.92, 30);

EXPECT_CALL(mockOrderRepo,  findById(ORDER_ID)).WillOnce(Return(inOrder));
EXPECT_CALL(mockSampleRepo, findById(SAMPLE_ID)).WillOnce(Return(inSample));
EXPECT_CALL(mockOrderRepo,  update(_)).Times(1);
EXPECT_CALL(mockSampleRepo, update(_)).Times(0);

ctrl->approveOrder(ORDER_ID);

ASSERT_FALSE(prodQueue.empty());
EXPECT_EQ(prodQueue.front().getActualProd(), 206);
EXPECT_EQ(prodQueue.front().getOrderId(),    ORDER_ID);
```

---

## vcxproj 변경 사항

없음. `OrderController.cpp`와 `test_order_controller.cpp` 모두 Phase 4에서 이미 등록됨.

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 누적 59개 통과 (Phase 0~4의 46개 + Phase 5의 7개 + reserved 추가 6개) |
| 재고 분기 | `isStockEnough` 경계값(stock == qty)이 CONFIRMED 로 처리됨 |
| MVC 분리 | OrderController에 `std::cout` 없음 |
| 생산 공식 | shortage → actualProd 206 이 prodQueue 에 실제로 기록됨 |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| 상태 확인 순서 | `findById(orderId)` → 상태 확인 → `findById(sampleId)` 순서 지켜야 테스트 9가 통과 |
| `sampleRepo.update` 호출 여부 | CONFIRMED 경로: 필수 (reserveQty 변경 반영). PRODUCING 경로: 미호출 |
| `prodQueue` 직접 검증 | queue 는 Mock 이 아닌 실 객체이므로 `prodQueue.front()` 로 직접 접근 가능 |
| `SaveArg<0>` 초기값 | `capturedOrder` 를 `inOrder` 로 초기화해야 update 미호출 시 쓰레기 값 참조 방지 |
| 역직렬화 생성자 활용 | 테스트 9에서 CONFIRMED 상태 Order 를 만들 때 6-인자 역직렬화 생성자 사용 |
| `ASSERT_FALSE` vs `EXPECT_FALSE` | prodQueue 비어있는지 먼저 `ASSERT_FALSE(prodQueue.empty())` 로 확인 후 `.front()` 접근 (비어있으면 UB) |