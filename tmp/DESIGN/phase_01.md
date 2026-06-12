# Phase 1 상세 구현 계획 — Model 계층

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md)
> 대상 Phase: 1-1 (Sample) · 1-2 (Order) · 1-3 (ProductionJob)
> 총 테스트: 18개 (각 서브 Phase 6개)

---

## 개요

Phase 1은 순수 도메인 객체(Model)만 구현한다.
파일 I/O, Controller, 외부 의존성 없이 **값 저장·계산·예외** 만 다룬다.
Mock이 필요 없고, 테스트가 가장 단순한 단계다.

---

## 변경 파일 목록

### 구현 파일 (code-writer 작업)

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/model/OrderStatus.h` | 수정 | `toString` / `fromString` 유틸 추가 |
| `src/model/Sample.h` | 수정 | 메서드 시그니처 확정 |
| `src/model/Sample.cpp` | 신규 | 실제 구현 |
| `src/model/Order.h` | 수정 | `isMonitored` / `generateOrderId` 추가 |
| `src/model/Order.cpp` | 신규 | 실제 구현 |
| `src/model/ProductionJob.h` | 수정 | 시간 기반 메서드 추가 |
| `src/model/ProductionJob.cpp` | 신규 | 실제 구현 |

### 테스트 파일 (test-writer 작업)

| 파일 | 테스트 수 |
|------|----------|
| `test/test_sample.cpp` | 6 |
| `test/test_order.cpp` | 6 |
| `test/test_production_job.cpp` | 6 |

### 프로젝트 파일

`SemiProdiction.vcxproj` — 신규 `.cpp` 3개(Release) + 테스트 `.cpp` 3개(Debug) 추가

---

## Phase 1-1 · Sample 모델

### 목표

시료(Sample)의 데이터 보관과 재고 조작이 올바르게 동작한다.
`OrderStatus` 열거형과 문자열 변환도 이 단계에서 완성한다.

---

### OrderStatus 설계

```
파일: src/model/OrderStatus.h
```

#### 열거형

| 값 | 의미 |
|----|------|
| `RESERVED` | 예약(접수) 완료 |
| `CONFIRMED` | 승인 완료 (출고 대기) |
| `PRODUCING` | 생산 중 |
| `RELEASE` | 출고 완료 |
| `REJECTED` | 거절됨 |

#### 추가 유틸리티

```
std::string toString(OrderStatus status)
    RESERVED   → "RESERVED"
    CONFIRMED  → "CONFIRMED"
    PRODUCING  → "PRODUCING"
    RELEASE    → "RELEASE"
    REJECTED   → "REJECTED"

OrderStatus fromString(const std::string& s)
    알 수 없는 문자열 → std::invalid_argument 예외
```

---

### Sample 클래스 설계

```
파일: src/model/Sample.h + Sample.cpp
```

#### 멤버 변수

| 변수명 | 타입 | 설명 |
|--------|------|------|
| `m_id` | `std::string` | 시료 ID (예: "S-001") |
| `m_name` | `std::string` | 시료명 (예: "실리콘 웨이퍼-8인치") |
| `m_avgProdTime` | `double` | 단위당 평균 생산시간 (분) |
| `m_yield` | `double` | 수율 (0 초과 ~ 1.0 이하) |
| `m_stock` | `int` | 현재 재고 수량 |

#### 생성자

```
Sample(id, name, avgProdTime, yield, stock)
```

유효성 검사 (위반 시 `std::invalid_argument`):
- `yield`: 0 초과 · 1.0 이하
- `avgProdTime`: 0 초과
- `stock`: 0 이상

#### 메서드

| 메서드 | 반환 | 설명 |
|--------|------|------|
| `getId()` | `const string&` | |
| `getName()` | `const string&` | |
| `getAvgProdTime()` | `double` | |
| `getYield()` | `double` | |
| `getStock()` | `int` | |
| `addStock(int qty)` | `void` | 재고 증가. qty ≤ 0이면 `invalid_argument` |
| `reduceStock(int qty)` | `void` | 재고 감소. qty > stock이면 `runtime_error` |
| `isStockEnough(int qty)` | `bool` | `m_stock >= qty` 이면 `true` |

모든 getter는 `const` 선언.

---

### Phase 1-1 테스트 케이스 (6개)

| # | 테스트명 | 검증 내용 |
|---|---------|----------|
| 1 | `SampleTest, Constructor_StoresAllValues` | id, name, avgProdTime, yield, stock 5개 getter 모두 확인 |
| 2 | `SampleTest, AddStock_IncreasesStock` | stock=100에 addStock(50) → stock=150 |
| 3 | `SampleTest, ReduceStock_DecreasesStock` | stock=100에 reduceStock(30) → stock=70 |
| 4 | `SampleTest, ReduceStock_WhenInsufficient_Throws` | stock=10에 reduceStock(20) → `runtime_error` |
| 5 | `SampleTest, IsStockEnough_BoundaryValue` | stock=100, qty=100 → `true` / qty=101 → `false` |
| 6 | `OrderStatusTest, ToString_And_FromString_RoundTrip` | 5개 상태값 → 문자열 → 다시 enum 변환 일치 |

---

## Phase 1-2 · Order 모델

### 목표

주문(Order)의 데이터 보관, 상태 전이, 모니터링 포함 여부, 주문번호 형식을 구현한다.

---

### Order 클래스 설계

```
파일: src/model/Order.h + Order.cpp
```

#### 멤버 변수

| 변수명 | 타입 | 설명 |
|--------|------|------|
| `m_orderId` | `std::string` | 주문번호 (ORD-YYYYMMDD-NNNN) |
| `m_sampleId` | `std::string` | 시료 ID |
| `m_customerName` | `std::string` | 고객명 |
| `m_quantity` | `int` | 주문 수량 |
| `m_status` | `OrderStatus` | 주문 상태 (기본값: `RESERVED`) |
| `m_createdAt` | `long long` | 생성 시각 (Unix timestamp) |

#### 생성자

```
Order(orderId, sampleId, customerName, quantity)
```

유효성 검사 (위반 시 `std::invalid_argument`):
- `quantity`: 1 이상
- `customerName`: 비어있으면 안 됨

생성 시 `m_status = OrderStatus::RESERVED`, `m_createdAt = 현재 시각`.

#### 메서드

| 메서드 | 반환 | 설명 |
|--------|------|------|
| `getOrderId()` | `const string&` | |
| `getSampleId()` | `const string&` | |
| `getCustomerName()` | `const string&` | |
| `getQuantity()` | `int` | |
| `getStatus()` | `OrderStatus` | |
| `getCreatedAt()` | `long long` | |
| `setStatus(OrderStatus)` | `void` | 상태 변경 |
| `isMonitored()` | `bool` | `REJECTED`이면 `false`, 나머지는 `true` |

#### 정적 유틸리티

```
static std::string generateOrderId(int sequence)
    → "ORD-YYYYMMDD-" + zero-padded 4자리 sequence
    → 예: generateOrderId(1) → "ORD-20260612-0001"
```

---

### Phase 1-2 테스트 케이스 (6개)

| # | 테스트명 | 검증 내용 |
|---|---------|----------|
| 1 | `OrderTest, Constructor_DefaultStatusIsReserved` | 생성 직후 `getStatus() == RESERVED` |
| 2 | `OrderTest, SetStatus_ChangesStatus` | setStatus(CONFIRMED) → getStatus() == CONFIRMED |
| 3 | `OrderTest, IsMonitored_ReturnsFalseForRejected` | status=REJECTED → `isMonitored() == false` |
| 4 | `OrderTest, IsMonitored_ReturnsTrueForActiveStatuses` | RESERVED·CONFIRMED·PRODUCING·RELEASE 각각 `true` |
| 5 | `OrderTest, GenerateOrderId_FormatsCorrectly` | `"ORD-"` 시작, `"-"` 포함, 4자리 시퀀스 suffix 확인 |
| 6 | `OrderTest, Constructor_NegativeQuantity_Throws` | quantity=0 → `invalid_argument` |

---

## Phase 1-3 · ProductionJob 모델

### 목표

생산 작업의 **실생산량 계산 공식**과 시간 기반 완료 여부를 구현한다.
공식이 PRD와 정확히 일치해야 한다.

---

### 핵심 공식

```
실생산량 = ceil(부족분 / (수율 × 0.9))
총 생산시간(분) = 평균 생산시간(분/개) × 실생산량
```

검증 케이스:

| 부족분 | 수율 | 계산 | 실생산량 |
|--------|------|------|---------|
| 170 | 0.92 | ceil(170 / 0.828) = ceil(205.31) | **206** |
| 150 | 0.88 | ceil(150 / 0.792) = ceil(189.39) | **190** |

---

### ProductionJob 클래스 설계

```
파일: src/model/ProductionJob.h + ProductionJob.cpp
```

#### 멤버 변수

| 변수명 | 타입 | 설명 |
|--------|------|------|
| `m_orderId` | `std::string` | 대상 주문 ID |
| `m_sampleId` | `std::string` | 대상 시료 ID |
| `m_shortage` | `int` | 부족분 (생산 목표) |
| `m_yield` | `double` | 수율 |
| `m_avgProdTime` | `double` | 단위당 평균 생산시간 (분) |
| `m_startTime` | `long long` | 생산 시작 Unix timestamp (생성 시 설정) |
| `m_actualProd` | `int` | 실생산량 (생성 시 공식으로 계산) |

#### 생성자

```
ProductionJob(orderId, sampleId, shortage, yield, avgProdTime)
```

생성 시 `m_actualProd` 계산 및 `m_startTime = 현재 시각` 설정.

#### 메서드

| 메서드 | 반환 | 설명 |
|--------|------|------|
| `getOrderId()` | `const string&` | |
| `getSampleId()` | `const string&` | |
| `getShortage()` | `int` | |
| `getActualProd()` | `int` | 계산된 실생산량 |
| `getTotalProdTime()` | `double` | `m_avgProdTime × m_actualProd` (분) |
| `getStartTime()` | `long long` | 시작 timestamp |
| `getCompletionTime()` | `long long` | `startTime + totalProdTime × 60` (초 단위) |
| `isCompleted(long long nowSec)` | `bool` | `nowSec >= getCompletionTime()` |
| `getCurrentProd(long long nowSec)` | `int` | 경과 시간 비례 생산량, `[0, actualProd]` 범위로 clamp |

#### isCompleted · getCurrentProd 에 시각을 매개변수로 받는 이유

테스트에서 실제 시간이 흐르기를 기다릴 수 없으므로,
호출자가 현재 시각을 주입한다. Production 환경에서는 `std::time(nullptr)` 전달.

---

### Phase 1-3 테스트 케이스 (6개)

| # | 테스트명 | 검증 내용 |
|---|---------|----------|
| 1 | `ProductionJobTest, ActualProd_Formula_Case1` | shortage=170, yield=0.92 → `getActualProd() == 206` |
| 2 | `ProductionJobTest, ActualProd_Formula_Case2` | shortage=150, yield=0.88 → `getActualProd() == 190` |
| 3 | `ProductionJobTest, TotalProdTime_IsAvgTimeMulActualProd` | avgProdTime=0.5, actualProd=206 → `getTotalProdTime() == 103.0` |
| 4 | `ProductionJobTest, IsCompleted_ReturnsFalseJustAfterStart` | `isCompleted(m_startTime)` → `false` (시작 시각엔 아직 미완료) |
| 5 | `ProductionJobTest, GetCompletionTime_IsGreaterThanStartTime` | `getCompletionTime() > getStartTime()` |
| 6 | `ProductionJobTest, GetCurrentProd_IsZeroJustAfterStart` | `getCurrentProd(m_startTime) == 0` |

---

## vcxproj 추가 항목

test-writer / code-writer 작업 완료 후 아래 항목을 `SemiProdiction.vcxproj`에 추가해야 한다.

### Debug (테스트, `Condition="'$(Configuration)'=='Debug'"`)

```
test\test_sample.cpp
test\test_order.cpp
test\test_production_job.cpp
```

### Release (구현, `Condition="'$(Configuration)'=='Release'"`)

```
src\model\Sample.cpp
src\model\Order.cpp
src\model\ProductionJob.cpp
```

> **주의**: `src/main.cpp` 는 이미 Release 조건으로 등록되어 있으므로 건드리지 않는다.

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 18개 통과 (Phase 0의 2개 포함 시 누적 20개) |
| 공식 정확성 | shortage=170, yield=0.92 → 실생산량 206 필수 |
| MVC 경계 | Model 파일에 `#include <iostream>` / `std::cout` 없음 |
| 예외 타입 | 입력 검증 → `invalid_argument`, 상태 위반 → `runtime_error` |
