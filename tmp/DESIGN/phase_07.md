# Phase 7 상세 구현 계획 — 모니터링 (MonitoringController + MonitorView)

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_06.md](./phase_06.md)
> 대상 Phase: 7 (MonitoringController + MonitorView)
> 총 테스트: 5개 (test_monitoring_controller.cpp 신규)

---

## 개요

Phase 7은 시스템 현황을 한눈에 볼 수 있는 **모니터링 기능**을 구현한다.
두 가지 정보를 제공한다.

1. **주문 건수 집계** — 상태별(RESERVED / CONFIRMED / PRODUCING / RELEASE) 주문 수. REJECTED는 유효하지 않은 주문이므로 제외.
2. **재고 상태** — 시료별 현재 재고와 예약 수량을 비교해 여유/부족/고갈 세 등급으로 분류.

기존에 헤더와 모델만 존재하며, 구현 파일이 없는 상태다.

---

## 기존 파일 현황

| 파일 | 상태 | 내용 |
|------|------|------|
| `src/controller/MonitoringController.h` | 기존 존재 | 인터페이스 정의 완료 |
| `src/controller/MonitoringController.cpp` | **없음** | 신규 작성 필요 |
| `src/view/MonitorView.h` | 기존 존재 | 인터페이스 정의 완료 |
| `src/view/MonitorView.cpp` | **없음** | 신규 작성 필요 |
| `src/model/MonitoringSummary.h` | 기존 존재 | StockStatus, OrderSummary 정의 완료 |
| `test/test_monitoring_controller.cpp` | **없음** | 신규 작성 필요 |

---

## 변경·신규 파일 목록

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/controller/MonitoringController.cpp` | 신규 | 전체 구현 |
| `src/view/MonitorView.cpp` | 신규 | 전체 구현 |
| `test/test_monitoring_controller.cpp` | 신규 | 테스트 5개 |
| `SemiProdiction.vcxproj` | 수정 | 위 .cpp 3개 추가 |

---

## 모델 확인

### MonitoringSummary.h (기존)

```cpp
enum class StockStatus { SUFFICIENT, SHORTAGE, DEPLETED };

struct OrderSummary {
    int totalReserved  = 0;
    int totalConfirmed = 0;
    int totalProducing = 0;
    int totalRelease   = 0;
};
```

### MonitoringController.h (기존)

```cpp
class MonitoringController {
public:
    explicit MonitoringController(IOrderRepository& orderRepo,
                                  ISampleRepository& sampleRepo);

    OrderSummary       getOrderSummary()  const;
    StockStatus        getStockStatus(const std::string& sampleId, int reservedQty) const;
    std::vector<Order> getActiveOrders()  const;

private:
    IOrderRepository& m_orderRepo;
    ISampleRepository& m_sampleRepo;
};
```

### MonitorView.h (기존)

```cpp
class MonitorView {
public:
    void showOrderSummary(const OrderSummary& summary)         const;
    void showActiveOrders(const std::vector<Order>& orders)    const;
    void showStockStatus(const Sample& sample, StockStatus status) const;
};
```

---

## MonitoringController.cpp 구현 설계

### getOrderSummary() const

```
1. orders = m_orderRepo.findAll()
2. OrderSummary summary (모두 0으로 초기화됨)
3. for each order:
       RESERVED  → summary.totalReserved++
       CONFIRMED → summary.totalConfirmed++
       PRODUCING → summary.totalProducing++
       RELEASE   → summary.totalRelease++
       REJECTED  → (무시)
4. return summary
```

REJECTED를 switch default에서 무시하면 되므로 별도 필터링 불필요.

---

### getStockStatus(sampleId, reservedQty) const

```
1. Sample sample = m_sampleRepo.findById(sampleId)
2. int stock = sample.getStock()

3. if (stock == 0)          return StockStatus::DEPLETED
   if (stock < reservedQty) return StockStatus::SHORTAGE
                             return StockStatus::SUFFICIENT
```

**재고 상태 판단 기준 (PRD)**

| 상태 | 조건 |
|------|------|
| DEPLETED (고갈) | stock == 0 |
| SHORTAGE (부족) | 0 < stock < reservedQty |
| SUFFICIENT (여유) | stock >= reservedQty |

**경계값**: stock == reservedQty 는 SUFFICIENT (재고가 예약량을 정확히 충당).
stock == 0 은 DEPLETED 우선 (SHORTAGE 조건보다 먼저 판단).

---

### getActiveOrders() const

```
1. orders = m_orderRepo.findAll()
2. result = []
3. for each order:
       if order.isMonitored() → result.push_back(order)
4. return result
```

`Order::isMonitored()` 는 REJECTED 상태를 제외한 모든 주문에서 true를 반환한다 (기존 구현).

---

## MonitorView.cpp 구현 설계

콘솔 출력 전용. 비즈니스 로직 없음.

### showOrderSummary(summary)

```
출력 형식 예시:
=== 주문 현황 ===
접수 대기 (RESERVED) :  3건
생산 중   (PRODUCING):  1건
출고 대기 (CONFIRMED):  2건
출고 완료 (RELEASE)  :  5건
```

### showActiveOrders(orders)

```
출력 형식 예시:
=== 활성 주문 목록 ===
주문번호               시료ID  고객명     수량  상태
ORD-20260612-0001  S-001  삼성전자  100  CONFIRMED
...
(비어있으면: "활성 주문이 없습니다.")
```

### showStockStatus(sample, status)

```
출력 형식 예시:
[여유] S-001 실리콘 웨이퍼  재고: 500ea
[부족] S-002 GaAs 웨이퍼   재고:  30ea
[고갈] S-003 SiC 웨이퍼    재고:   0ea
```

---

## 테스트 설계 (5개)

**파일**: `test/test_monitoring_controller.cpp` (신규)

**의존성**: MockOrderRepository, MockSampleRepository (기존 파일 재사용)

**공통 Fixture**

```cpp
class MonitoringControllerTest : public ::testing::Test {
protected:
    MockOrderRepository  mockOrderRepo;
    MockSampleRepository mockSampleRepo;
    std::unique_ptr<MonitoringController> ctrl;

    void SetUp() override {
        ctrl = std::make_unique<MonitoringController>(mockOrderRepo, mockSampleRepo);
    }
};
```

---

### 테스트 1 — 상태별 집계: REJECTED 1건 제외한 5건이 정확히 집계됨

```
GIVEN: 주문 6건
       RESERVED  1건
       CONFIRMED 2건
       PRODUCING 1건
       RELEASE   1건
       REJECTED  1건  ← 집계에서 제외
WHEN:  getOrderSummary()
THEN:  totalReserved  == 1
       totalConfirmed == 2
       totalProducing == 1
       totalRelease   == 1
```

GMock 설정:
```cpp
std::vector<Order> orders = {
    Order("ORD-001", "S-001", "고객A", 10),                               // RESERVED
    Order("ORD-002", "S-001", "고객B", 10, OrderStatus::CONFIRMED,  0LL),
    Order("ORD-003", "S-001", "고객C", 10, OrderStatus::CONFIRMED,  0LL),
    Order("ORD-004", "S-001", "고객D", 10, OrderStatus::PRODUCING,  0LL),
    Order("ORD-005", "S-001", "고객E", 10, OrderStatus::RELEASE,    0LL),
    Order("ORD-006", "S-001", "고객F", 10, OrderStatus::REJECTED,   0LL),
};

EXPECT_CALL(mockOrderRepo, findAll()).WillOnce(Return(orders));

auto summary = ctrl->getOrderSummary();

EXPECT_EQ(summary.totalReserved,  1);
EXPECT_EQ(summary.totalConfirmed, 2);
EXPECT_EQ(summary.totalProducing, 1);
EXPECT_EQ(summary.totalRelease,   1);
```

---

### 테스트 2 — 재고 = 예약량 (경계값) → SUFFICIENT

```
GIVEN: Sample S-001, stock=100, reservedQty=100
WHEN:  getStockStatus("S-001", 100)
THEN:  StockStatus::SUFFICIENT
```

GMock 설정:
```cpp
Sample s("S-001", "시료A", 5.0, 0.92, 100);  // stock=100

EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(s));

EXPECT_EQ(ctrl->getStockStatus("S-001", 100), StockStatus::SUFFICIENT);
```

---

### 테스트 3 — 재고 < 예약량 → SHORTAGE

```
GIVEN: Sample S-001, stock=30, reservedQty=100
WHEN:  getStockStatus("S-001", 100)
THEN:  StockStatus::SHORTAGE
```

GMock 설정:
```cpp
Sample s("S-001", "시료A", 5.0, 0.92, 30);  // stock=30

EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(s));

EXPECT_EQ(ctrl->getStockStatus("S-001", 100), StockStatus::SHORTAGE);
```

---

### 테스트 4 — 재고 = 0 → DEPLETED

```
GIVEN: Sample S-001, stock=0, reservedQty=0 (또는 임의)
WHEN:  getStockStatus("S-001", 50)
THEN:  StockStatus::DEPLETED
```

GMock 설정:
```cpp
Sample s("S-001", "시료A", 5.0, 0.92, 0);  // stock=0

EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(s));

// stock==0이면 reservedQty에 무관하게 DEPLETED
EXPECT_EQ(ctrl->getStockStatus("S-001", 50), StockStatus::DEPLETED);
```

---

### 테스트 5 — getActiveOrders: REJECTED 제외한 주문 반환

```
GIVEN: 주문 4건 (RESERVED 1, CONFIRMED 1, PRODUCING 1, REJECTED 1)
WHEN:  getActiveOrders()
THEN:  반환 목록 크기 == 3
       REJECTED 주문은 포함되지 않음
```

GMock 설정:
```cpp
std::vector<Order> allOrders = {
    Order("ORD-001", "S-001", "고객A", 10),                               // RESERVED
    Order("ORD-002", "S-001", "고객B", 10, OrderStatus::CONFIRMED, 0LL),
    Order("ORD-003", "S-001", "고객C", 10, OrderStatus::PRODUCING, 0LL),
    Order("ORD-004", "S-001", "고객D", 10, OrderStatus::REJECTED,  0LL),
};

EXPECT_CALL(mockOrderRepo, findAll()).WillOnce(Return(allOrders));

auto active = ctrl->getActiveOrders();

EXPECT_EQ(active.size(), 3u);
for (const auto& o : active)
    EXPECT_NE(o.getStatus(), OrderStatus::REJECTED);
```

---

## vcxproj 추가 항목

### 항상 컴파일 (Debug + Release) — Controller + View 블록에 추가

```xml
<ClCompile Include="src\controller\MonitoringController.cpp" />
<ClCompile Include="src\view\MonitorView.cpp" />
```

### Debug 전용 테스트

```xml
<ClCompile Include="test\test_monitoring_controller.cpp"
           Condition="'$(Configuration)'=='Debug'" />
```

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 누적 76개 통과 (기존 71개 + Phase 7의 5개) |
| REJECTED 제외 | getOrderSummary, getActiveOrders 모두 REJECTED 집계/반환 없음 |
| 경계값 | stock == reservedQty → SUFFICIENT, stock == 0 → DEPLETED |
| MVC 분리 | MonitoringController에 std::cout 없음, MonitorView에 비즈니스 로직 없음 |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| `getStockStatus` 호출 방식 | main.cpp 에서 `sample.getReservedQty()` 를 두 번째 인자로 전달. 시료 객체를 컨트롤러 내부에서 조회하므로 sampleRepo.findById 가 1회 호출됨 |
| DEPLETED 우선 판단 | `stock == 0` 을 먼저 검사해야 reservedQty == 0 인 경우에도 DEPLETED 를 올바르게 반환 |
| `getActiveOrders` 필터 | `Order::isMonitored()` 위임 — 필터 로직 중복 없음 |
| MonitorView 독립성 | MonitorView.cpp 는 단순 포맷 출력. 테스트 대상이 아님 (컴파일 검증만) |