# Phase 2 상세 구현 계획 — Repository 계층

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_01.md](./phase_01.md)
> 대상 Phase: 2-1 (인터페이스·Mock 보완) · 2-2 (JsonSampleRepository) · 2-3 (JsonOrderRepository)
> 총 테스트: 12개 (2-1: 1개, 2-2: 6개, 2-3: 5개)

---

## 개요

Phase 2는 데이터를 JSON 파일에 읽고 쓰는 Repository 계층을 구현한다.
Controller는 이 인터페이스만 바라보고, 테스트에서는 Mock으로 대체한다.

**Phase 1과의 차이**: 파일 I/O가 등장한다. 테스트 격리(임시 파일)가 핵심이다.

---

## 사전 작업 — nlohmann/json 라이브러리 설치

JSON 파싱에 **nlohmann/json** 헤더 온리 라이브러리를 사용한다.

### 설치 방법 (둘 중 하나 선택)

**방법 A — 단일 헤더 직접 추가 (권장)**
1. [nlohmann/json 릴리즈](https://github.com/nlohmann/json/releases) 에서 `json.hpp` 다운로드
2. `SemiProdiction/src/lib/json.hpp` 에 저장
3. vcxproj 수정 불필요 (헤더 온리)

**방법 B — NuGet 패키지**
1. Visual Studio → 도구 → NuGet 패키지 관리
2. `nlohmann.json` 검색 후 설치

사용 시: `#include "src/lib/json.hpp"` (방법 A) 또는 `#include <nlohmann/json.hpp>` (방법 B)

---

## 변경 파일 목록

### 인터페이스 수정 (기존 스텁에 메서드 추가)

| 파일 | 변경 내용 |
|------|----------|
| `src/repository/ISampleRepository.h` | `findByName` 추가 |
| `src/repository/IOrderRepository.h` | `findByStatus` 추가 |
| `test/mock/MockSampleRepository.h` | `findByName` Mock 추가 |
| `test/mock/MockOrderRepository.h` | `findByStatus` Mock 추가 |

### 모델 수정 (역직렬화용 생성자)

| 파일 | 변경 내용 |
|------|----------|
| `src/model/Order.h` | 역직렬화용 생성자 선언 추가 |
| `src/model/Order.cpp` | 역직렬화용 생성자 구현 추가 |

### Repository 구현 (신규)

| 파일 | 작업 |
|------|------|
| `src/repository/JsonSampleRepository.cpp` | 신규 생성 |
| `src/repository/JsonOrderRepository.cpp` | 신규 생성 |

### 테스트 파일 (신규)

| 파일 | 테스트 수 |
|------|----------|
| `test/test_mock_compile.cpp` | 1 |
| `test/test_json_sample_repository.cpp` | 6 |
| `test/test_json_order_repository.cpp` | 5 |

---

## Phase 1 소급 변경 — Sample 모델에 reservedQty 추가

`reservedQty`를 JSON에 저장하려면 Sample 모델이 이 값을 보관해야 한다.
Phase 1에서 구현한 `Sample.h / Sample.cpp`를 아래와 같이 수정한다.

### Sample.h 추가 항목

```cpp
// 멤버 변수 추가
int m_reservedQty = 0;

// 메서드 추가
int  getReservedQty()   const;              // 예약 수량 조회
int  getAvailableStock() const;             // stock - reservedQty
void addReservedQty(int quantity);          // 승인 시 예약 수량 증가
void releaseReservedQty(int quantity);      // 출고 시 예약 수량 감소
```

### Sample 생성자 변경

기존 5인자 생성자에 `reservedQty` 추가 (기본값 0):
```cpp
Sample(const std::string& id, const std::string& name,
       double avgProdTime, double yield, int stock,
       int reservedQty = 0);
```
기본값 0이므로 기존 테스트 코드는 수정 불필요.

### 메서드 동작 규칙

```
getAvailableStock() → max(0, m_stock - m_reservedQty)

addReservedQty(qty):
  qty <= 0 → invalid_argument
  m_reservedQty += qty

releaseReservedQty(qty):
  qty > m_reservedQty → runtime_error("예약 수량 부족")
  m_reservedQty -= qty
```

### 영향 범위

| 파일 | 변경 사유 |
|------|----------|
| `src/model/Sample.h` | 멤버·메서드 추가 |
| `src/model/Sample.cpp` | 생성자·메서드 구현 추가 |
| `src/repository/JsonSampleRepository.cpp` | JSON 직렬화·역직렬화에 reservedQty 포함 |
| Phase 5 (OrderController::approve) | 승인 시 addReservedQty 호출 |
| Phase 8 (ReleaseController::release) | 출고 시 releaseReservedQty + reduceStock 호출 |

---

## 인터페이스 변경 설계

### ISampleRepository — `findByName` 추가

현재 스텁에 누락된 메서드. Phase 3 (SampleController 이름 검색)에서 필요하다.

```
추가할 선언:
virtual std::vector<Sample> findByName(const std::string& keyword) = 0;
```

동작: `keyword` 가 시료명에 부분 포함되면 결과에 포함 (대소문자 구분 없어도 됨).

### IOrderRepository — `findByStatus` 추가

현재 스텁에 누락된 메서드. Phase 5·8 (RESERVED 목록, CONFIRMED 목록)에서 필요하다.

```
추가할 선언:
virtual std::vector<Order> findByStatus(OrderStatus status) = 0;
```

### Mock 클래스 업데이트

두 Mock 파일에 각각 추가:
```cpp
// MockSampleRepository.h
MOCK_METHOD(std::vector<Sample>, findByName, (const std::string& keyword), (override));

// MockOrderRepository.h
MOCK_METHOD(std::vector<Order>, findByStatus, (OrderStatus status), (override));
```

---

## Order 역직렬화용 생성자

JSON 파일에서 주문을 복원할 때, 저장된 status와 createdAt을 그대로 설정해야 한다.
기존 생성자는 status를 RESERVED로, createdAt을 현재 시각으로 강제하므로 별도 생성자가 필요하다.

### 추가할 선언 (Order.h)

```cpp
// JSON 역직렬화 전용 — Repository 내부에서만 사용
Order(const std::string& orderId, const std::string& sampleId,
      const std::string& customerName, int quantity,
      OrderStatus status, long long createdAt);
```

### 구현 규칙 (Order.cpp)

- 유효성 검사 없음 (파일에서 읽은 데이터는 이미 저장 시 검증됨)
- m_status, m_createdAt 을 인자로 직접 설정

---

## JSON 파일 형식

### data/samples.json

```json
[
  {
    "id":          "S-001",
    "name":        "실리콘 웨이퍼-8인치",
    "avgProdTime": 0.5,
    "yield":       0.92,
    "stock":       480,
    "reservedQty": 200
  }
]
```

`reservedQty`: CONFIRMED·PRODUCING 상태 주문에 묶인 수량 합계.
실사용 가능 재고 = `stock - reservedQty`.

| 상황 | stock | reservedQty | 실가용 재고 |
|------|-------|------------|------------|
| 여유 | 480 | 0 | 480 |
| 일부 예약 | 480 | 200 | 280 |
| 전량 예약 | 480 | 480 | 0 |

#### reservedQty 변경 시점

| 이벤트 | 변화 |
|--------|------|
| 주문 승인 → CONFIRMED | `+= orderQty` |
| 주문 승인 → PRODUCING | `+= orderQty` |
| 주문 거절 → REJECTED | 변화 없음 (RESERVED는 예약 미반영) |
| 출고 → RELEASE | `-= orderQty` (stock도 동시에 차감) |
| 생산 완료 → CONFIRMED | 변화 없음 (이미 반영됨) |

### data/orders.json

```json
[
  {
    "orderId":      "ORD-20260612-0001",
    "sampleId":     "S-001",
    "customerName": "삼성전자",
    "quantity":     200,
    "status":       "RESERVED",
    "createdAt":    1749650000
  }
]
```

---

## Phase 2-1 · 인터페이스 및 Mock 보완

### 목표

위에서 정의한 인터페이스 변경과 Mock 업데이트를 적용하고,
Mock이 정상적으로 EXPECT_CALL과 함께 동작함을 확인한다.

### 테스트 케이스 (1개)

**파일**: `test/test_mock_compile.cpp`

| # | 테스트명 | 검증 내용 |
|---|---------|----------|
| 1 | `MockCompileTest, SampleAndOrderMockExpectCallWorks` | `MockSampleRepository`·`MockOrderRepository` 각각 EXPECT_CALL + WillOnce(Return) 호출 정상 동작 |

테스트 내용:
- MockSampleRepository: `findAll()` → `Return(빈 vector)` 설정 후 호출, 반환값 확인
- MockOrderRepository: `getNextSequence()` → `Return(1)` 설정 후 호출, 반환값 `== 1` 확인

---

## Phase 2-2 · JsonSampleRepository

### 목표

시료 데이터를 JSON 파일에 저장·조회·수정·검색한다.
테스트마다 독립된 임시 파일을 사용하여 `data/samples.json` 오염을 방지한다.

### 구현 설계

#### 생성자

```
JsonSampleRepository(const std::string& filePath)
  m_filePath = filePath
  파일이 없거나 비어있으면 "[]" 로 초기화
```

#### load() const — private

```
m_filePath 파일을 읽어 JSON 배열 파싱
각 객체 → Sample 객체 변환 후 vector 반환
파일이 비어있거나 없으면 빈 vector 반환
```

#### persist(vector<Sample>) — private

```
각 Sample → JSON 객체 변환
JSON 배열 직렬화 후 m_filePath 에 덮어쓰기
```

#### save(Sample)

```
samples = load()
samples에 추가
persist(samples)
```

#### findById(id) — 없으면 runtime_error

```
samples = load()
id 일치 항목 반환
없으면 throw runtime_error("시료를 찾을 수 없습니다: " + id)
```

#### findAll()

```
return load()
```

#### update(Sample)

```
samples = load()
id 일치 항목을 새 Sample로 교체
persist(samples)
id 없으면 throw runtime_error
```

#### exists(id)

```
samples = load()
id 일치 항목 있으면 true, 없으면 false
```

#### findByName(keyword)

```
samples = load()
name.find(keyword) != string::npos 인 항목만 반환
```

### 테스트 Fixture

```cpp
class JsonSampleRepositoryTest : public ::testing::Test {
protected:
    std::string m_tempFile = "temp_test_samples.json";

    void SetUp() override {
        std::ofstream f(m_tempFile);
        f << "[]";
    }
    void TearDown() override {
        std::remove(m_tempFile.c_str());
    }
};
```

### 테스트 케이스 (6개)

**파일**: `test/test_json_sample_repository.cpp`

| # | 테스트명 | 검증 내용 |
|---|---------|----------|
| 1 | `JsonSampleRepositoryTest, SaveAndFindById_ReturnsSameSample` | save 후 findById → id·name·stock 일치 |
| 2 | `JsonSampleRepositoryTest, FindById_NotFound_Throws` | 없는 id → `runtime_error` |
| 3 | `JsonSampleRepositoryTest, SaveTwo_FindAll_ReturnsBoth` | save 2개 → findAll().size()==2 |
| 4 | `JsonSampleRepositoryTest, Update_ReflectsChange` | save → stock 변경 후 update → findById로 변경값 확인 |
| 5 | `JsonSampleRepositoryTest, FindByName_PartialMatch` | "웨이퍼" 검색 → "실리콘 웨이퍼-8인치" 포함, "질화갈륨" 미포함 |
| 6 | `JsonSampleRepositoryTest, Persistence_ReloadRetainsData` | save 후 **새 인스턴스**로 findAll → 데이터 존재 확인 |

테스트 6번 핵심:
```
repo1.save(sample)           // 첫 번째 인스턴스로 저장
JsonSampleRepository repo2(m_tempFile)  // 새 인스턴스 생성
EXPECT_EQ(repo2.findAll().size(), 1)    // 데이터 유지 확인
```

---

## Phase 2-3 · JsonOrderRepository

### 목표

주문 데이터를 JSON 파일에 저장·조회·수정·상태별 조회한다.
`getNextSequence()`는 재실행 후에도 중복 없이 증가하는 시퀀스를 반환한다.

### 구현 설계

#### getNextSequence()

기존 주문 수 기반으로 채번한다.

```
orders = load()
return (int)orders.size() + 1
```

근거: 주문은 파일에서 삭제되지 않으므로 size+1은 항상 유일하다.

#### findByStatus(OrderStatus status)

```
orders = load()
status 일치 항목만 반환
```

#### 나머지 메서드 (save, findById, findAll, update, exists)

JsonSampleRepository와 동일한 패턴. Sample 대신 Order로 변경.

역직렬화 시 Order의 6인자 생성자(역직렬화용) 사용:
```
status 필드: fromString(json["status"]) 으로 OrderStatus 복원
createdAt 필드: json["createdAt"] 으로 long long 복원
```

### 테스트 Fixture

```cpp
class JsonOrderRepositoryTest : public ::testing::Test {
protected:
    std::string m_tempFile = "temp_test_orders.json";

    void SetUp() override {
        std::ofstream f(m_tempFile);
        f << "[]";
    }
    void TearDown() override {
        std::remove(m_tempFile.c_str());
    }
    // 테스트용 Order 헬퍼
    Order makeOrder(const std::string& id, int qty = 100) {
        return Order(id, "S-001", "테스트고객", qty);
    }
};
```

### 테스트 케이스 (5개)

**파일**: `test/test_json_order_repository.cpp`

| # | 테스트명 | 검증 내용 |
|---|---------|----------|
| 1 | `JsonOrderRepositoryTest, SaveAndFindById_ReturnsSameOrder` | save 후 findById → orderId·customerName·quantity 일치 |
| 2 | `JsonOrderRepositoryTest, FindByStatus_FiltersCorrectly` | RESERVED 1건·CONFIRMED 1건 저장 후 findByStatus(CONFIRMED) → 1건만 반환 |
| 3 | `JsonOrderRepositoryTest, GetNextSequence_Increments` | 빈 파일: getNextSequence()==1 / save 1건 후: getNextSequence()==2 |
| 4 | `JsonOrderRepositoryTest, Update_StatusChange_IsReflected` | RESERVED 저장 → setStatus(CONFIRMED) 후 update → findById로 CONFIRMED 확인 |
| 5 | `JsonOrderRepositoryTest, Persistence_ReloadRetainsData` | save 후 새 인스턴스로 findAll → orderId·status 일치 확인 |

테스트 5번 핵심 (status 영속성):
```
repo1.save(order)
order.setStatus(OrderStatus::CONFIRMED)
repo1.update(order)
JsonOrderRepository repo2(m_tempFile)
auto loaded = repo2.findById(order.getOrderId())
EXPECT_EQ(loaded.getStatus(), OrderStatus::CONFIRMED)
```

---

## vcxproj 추가 항목

### 항상 컴파일 (Debug + Release 모두)

```xml
<ClCompile Include="src\repository\JsonSampleRepository.cpp" />
<ClCompile Include="src\repository\JsonOrderRepository.cpp" />
```

### Debug 전용 테스트

```xml
<ClCompile Include="test\test_mock_compile.cpp" Condition="'$(Configuration)'=='Debug'" />
<ClCompile Include="test\test_json_sample_repository.cpp" Condition="'$(Configuration)'=='Debug'" />
<ClCompile Include="test\test_json_order_repository.cpp" Condition="'$(Configuration)'=='Debug'" />
```

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 누적 32개 통과 (Phase 0·1의 20개 + Phase 2의 12개) |
| 파일 격리 | 테스트 종료 후 `temp_test_*.json` 파일 삭제 확인 |
| 실제 데이터 | `data/samples.json`, `data/orders.json` 내용 변경 없음 |
| 영속성 | 새 인스턴스로 재로드 시 저장 데이터 복원 |
| 인터페이스 호환 | Mock이 새 메서드(findByName·findByStatus) 포함 |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| `reservedQty` 일관성 | 승인·출고 시 OrderController·ReleaseController가 반드시 addReservedQty / releaseReservedQty를 호출해야 함. 누락 시 재고 오차 발생 |
| `getAvailableStock` 음수 방지 | 비정상 데이터 대비 `max(0, stock - reservedQty)` 처리 |
| `findByName` 대소문자 | 현재 설계는 대소문자 구분. Phase 3에서 요구 변경 시 수정 |
| `getNextSequence` 방식 | 주문 건수 기반 채번. 주문 삭제 기능이 추가될 경우 별도 시퀀스 파일로 전환 필요 |
| Order 역직렬화 생성자 | `public` 선언 (단순함 우선). 설계상 Repository 외부에서 호출하지 않도록 주석 명시 |
| nlohmann/json 예외 | JSON 파싱 실패 시 `nlohmann::json::exception` 발생. Repository에서 잡아서 `runtime_error`로 변환 권장 |
