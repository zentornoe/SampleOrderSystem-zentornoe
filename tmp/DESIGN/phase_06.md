# Phase 6 상세 구현 계획 — 생산라인 (ProductionController)

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_05.md](./phase_05.md)
> 대상 Phase: 6 (ProductionController::tick / getProgress)
> 총 테스트: 6개 (test_production_controller.cpp 신규)

---

## 개요

Phase 6은 FIFO 방식으로 `ProductionJob` 을 순서대로 처리하는 **생산라인 컨트롤러**를 구현한다.
메인 루프마다 `tick(nowSec)` 을 호출하면 완료된 작업을 자동으로 감지하고
주문을 CONFIRMED 으로 전환한 뒤 생산된 재고를 시료에 추가한다.

**상태 전이**

```
생산라인 PRODUCING 주문 → tick() → isCompleted(nowSec) == true
    → order.completeProduction()   // PRODUCING → CONFIRMED
    → sample.addStock(actualProd)
    → prodQueue.pop()              // 다음 작업으로 자동 이동
```

**테스트 가능성 설계 원칙**

- `ProductionJob::m_startTime` 은 생성자에서 `std::time(nullptr)` 로 고정된다.
- `isCompleted(nowSec)` = `nowSec >= completionTime` 이므로,
  `nowSec` 를 매개변수로 주입하면 시간 의존성 없이 테스트할 수 있다.
- `avgProdTime = 0.0` 인 job 은 `completionTime == startTime` 이므로
  `tick(startTime)` 로 즉시 완료 처리할 수 있다.
  단, `getProgress` 에서 `getCurrentProd` 의 division-by-zero 를 피하려면
  `getProgress` 테스트에서는 `avgProdTime > 0` 을 사용해야 한다.

---

## 변경·신규 파일 목록

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/controller/ProductionController.h` | 수정 | `tick()`, `getProgress()` 선언 추가 |
| `src/controller/ProductionController.cpp` | 신규 | 전체 구현 |
| `test/test_production_controller.cpp` | 신규 | 테스트 6개 |
| `SemiProdiction.vcxproj` | 수정 | 위 `.cpp` 2개 추가 |

---

## ProductionController.h 변경

현재 헤더에는 `tick()` 과 `getProgress()` 가 없다. 두 메서드를 추가한다.

### 변경 후 전체 public 인터페이스

```cpp
explicit ProductionController(std::queue<ProductionJob>& prodQueue,
                               IOrderRepository& orderRepo,
                               ISampleRepository& sampleRepo);

void   tick(long long nowSec);              // 완료 감지 및 상태 전이 (메인 루프 호출용)
double getProgress(long long nowSec) const; // 현재 작업 진행률 0~100 반환

bool            hasJob()       const;
ProductionJob   peekNextJob()  const;
void            completeCurrentJob();       // 단순 pop (내부/수동 용도)
std::queue<ProductionJob> getQueue() const;
```

---

## ProductionController.cpp 구현 설계

### tick(long long nowSec)

```
1. if (m_prodQueue.empty()) → return (no-op, 테스트 6 검증 대상)

2. const ProductionJob& front = m_prodQueue.front()

3. if (!front.isCompleted(nowSec)) → return (아직 생산 중)

4. // 완료 처리
   Order order = m_orderRepo.findById(front.getOrderId())
   order.completeProduction()           // PRODUCING → CONFIRMED
   m_orderRepo.update(order)

   Sample sample = m_sampleRepo.findById(front.getSampleId())
   sample.addStock(front.getActualProd())  // 생산된 수량 재고 추가
   m_sampleRepo.update(sample)

5. m_prodQueue.pop()  // 다음 작업으로 이동 (FIFO)
```

**주의**: `order.completeProduction()` 은 `Order.cpp:49` 에서 PRODUCING 이 아닌 상태에서
호출 시 `logic_error` 를 발생시킨다. 따라서 prodQueue 에 있는 주문은 반드시 PRODUCING 상태여야 한다.
이 불변식은 `OrderController::approveOrder` 에서 보장한다.

---

### getProgress(long long nowSec) const

```
1. if (m_prodQueue.empty()) → return 0.0

2. const ProductionJob& front = m_prodQueue.front()

3. if (front.getActualProd() == 0) → return 0.0
   (zero-division 방지)

4. double ratio = front.getCurrentProd(nowSec) / (double)front.getActualProd()
   return std::min(100.0, ratio * 100.0)
```

---

### hasJob() const

```
return !m_prodQueue.empty()
```

---

### peekNextJob() const

```
if (m_prodQueue.empty())
    throw std::runtime_error("생산 큐가 비어 있습니다.")
return m_prodQueue.front()
```

---

### completeCurrentJob()

```
if (!m_prodQueue.empty())
    m_prodQueue.pop()
```

단순 pop. 상태 전이는 tick() 이 담당한다.

---

### getQueue() const

```
return m_prodQueue
```

---

## 시간 주입 패턴 정리

| 시나리오 | avgProdTime | nowSec | isCompleted 결과 | 안전 여부 |
|----------|-------------|--------|-----------------|-----------|
| 즉시 완료 테스트 (tick) | 0.0 | job.getStartTime() | true (elapsed=0, 나누기 발생 전 return) | ✅ |
| 즉시 완료 테스트 (tick) | 0.0 | 임의 미래 시각 | true (elapsed>0, totalSec=0 → division by zero) | ❌ — tick은 안전하지만 getProgress에서 UB |
| 미완료 테스트 | 양수 | startTime - 1 | false | ✅ |
| 진행률 테스트 | 양수 | startTime + (경과 초) | false or true | ✅ |
| FIFO 완료 (tick만) | 0.0 | 임의 미래 시각 | true | ✅ tick은 getCurrentProd 미호출 |

**결론**: tick() 전용 테스트(3, 4, 6)는 `avgProdTime=0.0`, `nowSec=임의` 조합 사용 가능.
getProgress 테스트(5)는 반드시 `avgProdTime > 0` 사용.

---

## 테스트 설계 (6개)

**파일**: `test/test_production_controller.cpp` (신규)

**공통 Fixture**

```cpp
class ProductionControllerTest : public ::testing::Test {
protected:
    MockOrderRepository  mockOrderRepo;
    MockSampleRepository mockSampleRepo;
    std::queue<ProductionJob> prodQueue;
    std::unique_ptr<ProductionController> ctrl;

    void SetUp() override {
        ctrl = std::make_unique<ProductionController>(
            prodQueue, mockOrderRepo, mockSampleRepo);
    }

    // avgProdTime=0.0 → completionTime == startTime → tick(startTime) 로 즉시 완료
    ProductionJob makeInstantJob(const std::string& orderId,
                                  const std::string& sampleId = "S-001")
    {
        return ProductionJob(orderId, sampleId, 1, 0.92, 0.0);
    }
};
```

---

### 테스트 1 — 초기 상태: 큐 비어있고 idle

```
GIVEN: ProductionController 생성 직후 (빈 prodQueue)
THEN:  ctrl->hasJob()          == false
       ctrl->getQueue().empty() == true
       ctrl->getProgress(any)   == 0.0
```

GMock 설정:
```cpp
// mock 기대 없음
EXPECT_FALSE(ctrl->hasJob());
EXPECT_TRUE(ctrl->getQueue().empty());
EXPECT_DOUBLE_EQ(ctrl->getProgress(std::time(nullptr)), 0.0);
```

---

### 테스트 2 — 작업 추가 시 즉시 처리 대기 시작

```
GIVEN: prodQueue에 job 1개 push
THEN:  ctrl->hasJob()                     == true
       ctrl->peekNextJob().getOrderId()   == "ORD-001"
```

GMock 설정:
```cpp
prodQueue.push(makeInstantJob("ORD-001"));

// mock 기대 없음 — 상태 조회만
EXPECT_TRUE(ctrl->hasJob());
EXPECT_EQ(ctrl->peekNextJob().getOrderId(), "ORD-001");
```

---

### 테스트 3 — FIFO 처리 순서 (3개 작업)

```
GIVEN: ORD-001, ORD-002, ORD-003 순서로 push (모두 avgProdTime=0.0)
WHEN:  tick() 3회 (각 직후 큐 front 확인)
THEN:  1회 tick → front == ORD-002
       2회 tick → front == ORD-003
       3회 tick → 큐 비어있음
```

GMock 설정:
```cpp
ProductionJob job1 = makeInstantJob("ORD-001");
ProductionJob job2 = makeInstantJob("ORD-002");
ProductionJob job3 = makeInstantJob("ORD-003");
prodQueue.push(job1); prodQueue.push(job2); prodQueue.push(job3);

// avgProdTime=0이므로 임의 미래 시각으로 모두 완료 (tick은 getCurrentProd 미호출)
long long futureTime = std::time(nullptr) + 1000000LL;

Order ord1("ORD-001", "S-001", "고객A", 10, OrderStatus::PRODUCING, 0LL);
Order ord2("ORD-002", "S-001", "고객B", 10, OrderStatus::PRODUCING, 0LL);
Order ord3("ORD-003", "S-001", "고객C", 10, OrderStatus::PRODUCING, 0LL);
Sample smp("S-001", "시료", 0.0, 0.92, 0);

EXPECT_CALL(mockOrderRepo, findById("ORD-001")).WillOnce(Return(ord1));
EXPECT_CALL(mockOrderRepo, findById("ORD-002")).WillOnce(Return(ord2));
EXPECT_CALL(mockOrderRepo, findById("ORD-003")).WillOnce(Return(ord3));
EXPECT_CALL(mockOrderRepo,  update(_)).Times(3);
EXPECT_CALL(mockSampleRepo, findById("S-001")).WillRepeatedly(Return(smp));
EXPECT_CALL(mockSampleRepo, update(_)).Times(3);

ctrl->tick(futureTime);
ASSERT_TRUE(ctrl->hasJob());
EXPECT_EQ(ctrl->peekNextJob().getOrderId(), "ORD-002");

ctrl->tick(futureTime);
ASSERT_TRUE(ctrl->hasJob());
EXPECT_EQ(ctrl->peekNextJob().getOrderId(), "ORD-003");

ctrl->tick(futureTime);
EXPECT_FALSE(ctrl->hasJob());
```

---

### 테스트 4 — 완료 시 CONFIRMED 전환 + 재고 addStock 호출

```
GIVEN: 1개 job push (avgProdTime=0.0, actualProd 계산: ceil(1 / (0.92*0.9)) = 2)
WHEN:  tick(job.getStartTime())  // completionTime == startTime → isCompleted = true
THEN:  orderRepo.update 에 전달된 order.getStatus() == CONFIRMED
       sampleRepo.update 에 전달된 sample.getStock() == 초기재고 + 2 (actualProd)
       큐 비어있음
```

GMock 설정:
```cpp
ProductionJob job = makeInstantJob("ORD-001");  // actualProd = 2
long long completionTime = job.getStartTime();  // avgProdTime=0 → startTime

Order inOrder("ORD-001", "S-001", "고객", 10, OrderStatus::PRODUCING, 0LL);
Sample inSample("S-001", "시료", 0.0, 0.92, 50);
Order  capturedOrder  = inOrder;
Sample capturedSample = inSample;

prodQueue.push(job);

EXPECT_CALL(mockOrderRepo,  findById("ORD-001")).WillOnce(Return(inOrder));
EXPECT_CALL(mockOrderRepo,  update(_)).WillOnce(SaveArg<0>(&capturedOrder));
EXPECT_CALL(mockSampleRepo, findById("S-001")).WillOnce(Return(inSample));
EXPECT_CALL(mockSampleRepo, update(_)).WillOnce(SaveArg<0>(&capturedSample));

ctrl->tick(completionTime);

EXPECT_EQ(capturedOrder.getStatus(), OrderStatus::CONFIRMED);
EXPECT_EQ(capturedSample.getStock(), 50 + job.getActualProd());
EXPECT_FALSE(ctrl->hasJob());
```

---

### 테스트 5 — 진행률이 0 이상 100 이하

```
GIVEN: avgProdTime=60.0, shortage=1, yield=0.92
       → actualProd = ceil(1 / 0.828) = 2
       → totalProdTime = 60 * 2 = 120 min = 7200 sec
WHEN:  getProgress(startTime)                → 0.0  (elapsed=0)
WHEN:  getProgress(startTime + 3600)         → 50.0 (halfway, getCurrentProd=1)
WHEN:  getProgress(startTime + 1000000)      → 100.0 (완료 후)
```

GMock 설정:
```cpp
ProductionJob job("ORD-001", "S-001", 1, 0.92, 60.0);
long long startTime = job.getStartTime();
prodQueue.push(job);

// mock 기대 없음 — getProgress는 repo 호출 없음
EXPECT_DOUBLE_EQ(ctrl->getProgress(startTime),          0.0);
EXPECT_DOUBLE_EQ(ctrl->getProgress(startTime + 3600LL), 50.0);
EXPECT_DOUBLE_EQ(ctrl->getProgress(startTime + 1000000LL), 100.0);
```

> `startTime + 3600` 에서 `getCurrentProd`:
> elapsed=3600, totalSec=7200, ratio=0.5, computed=2*0.5=1.0, min(2,1)=1
> getProgress = 1/2 * 100 = 50.0

---

### 테스트 6 — 빈 큐에서 tick() 호출 시 no-op

```
GIVEN: 빈 큐
WHEN:  tick(anyTime)
THEN:  orderRepo.findById  호출 0회
       orderRepo.update    호출 0회
       sampleRepo.findById 호출 0회
       sampleRepo.update   호출 0회
```

GMock 설정:
```cpp
EXPECT_CALL(mockOrderRepo,  findById(_)).Times(0);
EXPECT_CALL(mockOrderRepo,  update(_)).Times(0);
EXPECT_CALL(mockSampleRepo, findById(_)).Times(0);
EXPECT_CALL(mockSampleRepo, update(_)).Times(0);

ctrl->tick(std::time(nullptr));  // 빈 큐 → 아무 일도 없어야 함
```

---

## vcxproj 추가 항목

### 항상 컴파일 (Debug + Release)

```xml
<!-- Controller + View .cpp 블록에 추가 -->
<ClCompile Include="src\controller\ProductionController.cpp" />
```

### Debug 전용 테스트

```xml
<ClCompile Include="test\test_production_controller.cpp"
           Condition="'$(Configuration)'=='Debug'" />
```

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 누적 65개 통과 (Phase 0~5의 53개 + Phase 6의 6개 + 수정사항 6개) |
| FIFO | ORD-001 → ORD-002 → ORD-003 순서 보장 |
| 상태 전이 | tick() 완료 시 PRODUCING → CONFIRMED 자동 전환 |
| 재고 | tick() 완료 시 `addStock(actualProd)` 호출 |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| `tick` 시그니처 | `tick(long long nowSec)` — 시간 주입으로 테스트 가능. main.cpp 에서는 `tick(std::time(nullptr))` 로 호출 |
| `avgProdTime=0.0` 안전성 | tick() 은 `getCurrentProd` 미호출 → 안전. getProgress() 는 `getActualProd()==0` 가드 필요 |
| FIFO 보장 | `std::queue` 의 FIFO 특성 그대로 사용. 별도 정렬 로직 불필요 |
| `completeCurrentJob()` 역할 | 단순 pop, 상태 전이 없음. tick() 과 역할 분리 명확히 |
| 테스트 4 에서 `actualProd` | `shortage=1, yield=0.92` → `ceil(1/0.828) = ceil(1.207) = 2`. 초기 재고 50 → 52로 증가 검증 |
| 테스트 5 에서 50.0 검증 | `EXPECT_DOUBLE_EQ` 사용. 부동소수점 오차가 걱정되면 `EXPECT_NEAR(progress, 50.0, 0.01)` 대체 가능 |