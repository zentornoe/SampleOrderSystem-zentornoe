# Phase 9 상세 구현 계획 — 통합 및 main.cpp 조립

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_08.md](./phase_08.md)
> 대상 Phase: 9 (main.cpp 조립 + 통합 테스트)
> 총 테스트: 4개 (test_integration.cpp 신규, 실제 JSON 파일 사용)

---

## 개요

Phase 9는 Phase 1~8에서 구현한 모든 Controller / View / Repository를 `main.cpp`에 조립하고,
실제 JSON 파일을 사용하는 **종단간 통합 테스트(End-to-End Integration Test)**로 전체 흐름을 검증한다.

**메인 루프 흐름**

```
프로그램 시작
  ├─ Repository 생성 (JsonSampleRepository, JsonOrderRepository)
  ├─ 공유 prodQueue 생성
  ├─ Controller 조립 (Sample / Order / Production / Monitoring / Release)
  └─ while(true)
       ├─ prodCtrl.tick(nowSec)      ← 매 루프 생산 완료 자동 감지
       ├─ MainView.showMainMenu()
       ├─ MainView.getMenuChoice()
       └─ 메뉴별 핸들러 분기
            0: 종료
            1: 시료 관리  (SampleController + SampleView)
            2: 주문 접수  (OrderController + OrderView)
            3: 주문 승인/거절 (OrderController + OrderView)
            4: 생산 현황  (ProductionController + ProductionView)
            5: 모니터링   (MonitoringController + MonitorView)
            6: 출고 처리  (ReleaseController + OrderView)
```

---

## 기존 파일 현황

| 파일 | 상태 | 내용 |
|------|------|------|
| `src/main.cpp` | **미구현** | 빈 파일 또는 stub 상태 |
| `src/view/MainView.h/.cpp` | 헤더 존재, .cpp 미구현 | `showMainMenu()`, `getMenuChoice()`, `showError()` |
| `src/view/ProductionView.h` | 존재 (수정 필요) | `showProductionQueue(const std::queue<ProductionJob>&)` — `getQueue()`가 `vector` 반환으로 바뀌어 타입 불일치 |
| `src/view/ProductionView.cpp` | **없음** | 신규 작성 필요 |
| `src/repository/JsonSampleRepository.h/.cpp` | 완성 | 실제 파일 I/O |
| `src/repository/JsonOrderRepository.h/.cpp` | 완성 | 실제 파일 I/O |

---

## 변경·신규 파일 목록

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/view/ProductionView.h` | **수정** | `showProductionQueue` 인자를 `vector`로 변경 |
| `src/view/ProductionView.cpp` | **신규** | 전체 구현 |
| `src/view/MainView.cpp` | **신규** | 전체 구현 |
| `src/main.cpp` | **신규** | Controller 조립 + 메인 루프 |
| `test/test_integration.cpp` | **신규** | 통합 시나리오 4개 |
| `SemiProdiction.vcxproj` | 수정 | ProductionView.cpp / MainView.cpp 추가(항상); test_integration.cpp 추가(Debug) |

---

## ProductionView.h 수정

`getQueue()`가 `std::vector<ProductionJob>`을 반환하므로 View 시그니처도 맞춰 변경한다.

```cpp
#pragma once
#include <vector>
#include "../model/ProductionJob.h"

class ProductionView {
public:
    void showProductionQueue(const std::vector<ProductionJob>& jobs) const;
    void showCurrentJob(const ProductionJob& job) const;
    void showCompleteSuccess(const std::string& orderId) const;
    void showQueueEmpty() const;
};
```

---

## ProductionView.cpp 구현

```cpp
#include "ProductionView.h"
#include <iostream>
#include <iomanip>

void ProductionView::showProductionQueue(const std::vector<ProductionJob>& jobs) const {
    if (jobs.empty()) { showQueueEmpty(); return; }
    std::cout << "\n[생산 대기 현황]\n";
    std::cout << std::left
              << std::setw(20) << "주문 ID"
              << std::setw(10) << "시료 ID"
              << std::setw(12) << "실생산량"
              << "완료 예정\n";
    std::cout << std::string(60, '-') << "\n";
    for (size_t i = 0; i < jobs.size(); ++i) {
        const auto& j = jobs[i];
        std::cout << std::setw(20) << j.getOrderId()
                  << std::setw(10) << j.getSampleId()
                  << std::setw(12) << j.getActualProd()
                  << j.getCompletionTime() << "\n";
    }
}

void ProductionView::showCurrentJob(const ProductionJob& job) const {
    std::cout << "\n[현재 생산 중] " << job.getOrderId()
              << " | 시료: " << job.getSampleId()
              << " | 실생산량: " << job.getActualProd() << "\n";
}

void ProductionView::showCompleteSuccess(const std::string& orderId) const {
    std::cout << "[완료] 주문 " << orderId << " 생산이 완료되었습니다.\n";
}

void ProductionView::showQueueEmpty() const {
    std::cout << "현재 생산 대기 중인 작업이 없습니다.\n";
}
```

---

## MainView.cpp 구현

```cpp
#include "MainView.h"
#include <iostream>

void MainView::showMainMenu() const {
    std::cout << "\n==============================\n";
    std::cout << "  S-Semi 생산주문관리 시스템\n";
    std::cout << "==============================\n";
    std::cout << "  1. 시료 관리\n";
    std::cout << "  2. 주문 접수\n";
    std::cout << "  3. 주문 승인 / 거절\n";
    std::cout << "  4. 생산 현황\n";
    std::cout << "  5. 모니터링\n";
    std::cout << "  6. 출고 처리\n";
    std::cout << "  0. 종료\n";
    std::cout << "------------------------------\n";
    std::cout << "선택: ";
}

int MainView::getMenuChoice() const {
    int choice = -1;
    std::cin >> choice;
    if (std::cin.fail()) {
        std::cin.clear();
        std::cin.ignore(10000, '\n');
        return -1;
    }
    return choice;
}

void MainView::showError(const std::string& message) const {
    std::cout << "[오류] " << message << "\n";
}
```

---

## main.cpp 구현 설계

### 전체 구조

```cpp
#include <queue>
#include <ctime>
#include <iostream>
#include "src/model/ProductionJob.h"
#include "src/repository/JsonSampleRepository.h"
#include "src/repository/JsonOrderRepository.h"
#include "src/controller/SampleController.h"
#include "src/controller/OrderController.h"
#include "src/controller/ProductionController.h"
#include "src/controller/MonitoringController.h"
#include "src/controller/ReleaseController.h"
#include "src/view/MainView.h"
#include "src/view/SampleView.h"
#include "src/view/OrderView.h"
#include "src/view/ProductionView.h"
#include "src/view/MonitorView.h"

int main() {
    // 1. Repository 생성
    JsonSampleRepository sampleRepo("data/samples.json");
    JsonOrderRepository  orderRepo ("data/orders.json");

    // 2. 공유 생산 큐
    std::queue<ProductionJob> prodQueue;

    // 3. Controller 조립
    SampleController     sampleCtrl (sampleRepo);
    OrderController      orderCtrl  (orderRepo, sampleRepo, prodQueue);
    ProductionController prodCtrl   (prodQueue, orderRepo, sampleRepo);
    MonitoringController monCtrl    (orderRepo, sampleRepo, prodQueue);
    ReleaseController    releaseCtrl(orderRepo, sampleRepo);

    // 4. View
    MainView       mainView;
    SampleView     sampleView;
    OrderView      orderView;
    ProductionView prodView;
    MonitorView    monView;

    // 5. 메인 루프
    while (true) {
        long long nowSec = static_cast<long long>(std::time(nullptr));
        prodCtrl.tick(nowSec);   // 매 루프 생산 완료 자동 감지

        mainView.showMainMenu();
        int choice = mainView.getMenuChoice();

        try {
            switch (choice) {
            case 0: return 0;
            case 1: handleSampleMenu(sampleCtrl, sampleView, mainView); break;
            case 2: handleOrderReserve(orderCtrl, orderView, mainView); break;
            case 3: handleOrderApproveReject(orderCtrl, orderView, mainView); break;
            case 4: handleProduction(prodCtrl, prodView); break;
            case 5: handleMonitoring(monCtrl, monView, sampleCtrl); break;
            case 6: handleRelease(releaseCtrl, orderCtrl, orderView, mainView); break;
            default: mainView.showError("잘못된 메뉴 선택"); break;
            }
        } catch (const std::exception& e) {
            mainView.showError(e.what());
        }
    }
}
```

### 핸들러 함수별 설계

#### handleSampleMenu
```
서브 메뉴 출력:
  1. 시료 등록 (ID/이름/avgProdTime/yield/초기재고 입력 → sampleCtrl.registerSample)
  2. 전체 조회 → sampleView.showSampleList(sampleCtrl.getAllSamples())
  3. 이름 검색 → sampleView.showSampleList(sampleCtrl.searchByName(keyword))
  0. 돌아가기
```

#### handleOrderReserve
```
sampleId, customerName, quantity 입력
→ orderId = orderCtrl.reserveOrder(sampleId, customerName, quantity)
→ orderView.showReserveSuccess(orderId)
```

#### handleOrderApproveReject
```
대기 주문 목록 출력 (orderView.showOrderList)
orderId 입력 후 승인/거절 선택
승인: orderCtrl.approveOrder(orderId, nowSec) → orderView.showApproveSuccess
거절: orderCtrl.rejectOrder(orderId) → orderView.showRejectSuccess
```

#### handleProduction
```
if prodCtrl.hasJob():
    prodView.showCurrentJob(prodCtrl.peekNextJob())
prodView.showProductionQueue(prodCtrl.getQueue())
```

#### handleMonitoring
```
monView.showOrderSummary(monCtrl.getOrderSummary())
monView.showActiveOrders(monCtrl.getActiveOrders())
모든 시료 목록에 대해:
    StockStatus s = monCtrl.getStockStatus(sample.getId(), sample.getReservedQty())
    monView.showStockStatus(sample, s)
```

#### handleRelease
```
confirmed = releaseCtrl.getConfirmedOrders()
orderView.showOrderList(confirmed)
orderId 입력
releaseCtrl.releaseOrder(orderId)
```

---

## 통합 테스트 설계 (4개)

**파일**: `test/test_integration.cpp` (신규)

**핵심 설계 원칙**
- 실제 `JsonSampleRepository`, `JsonOrderRepository` 사용 (Mock 없음)
- 임시 파일 격리: `std::filesystem::temp_directory_path()` 사용, 테스트 종료 시 삭제
- 생산 완료 시뮬레이션: `avgProdTime=0.001` (극소) + `tick(job.getCompletionTime())` 주입
- 각 테스트는 독립적인 임시 파일을 사용

### 공통 Fixture

```cpp
class IntegrationTest : public ::testing::Test {
protected:
    std::filesystem::path samplePath;
    std::filesystem::path orderPath;

    std::unique_ptr<JsonSampleRepository> sampleRepo;
    std::unique_ptr<JsonOrderRepository>  orderRepo;
    std::queue<ProductionJob>             prodQueue;

    std::unique_ptr<SampleController>     sampleCtrl;
    std::unique_ptr<OrderController>      orderCtrl;
    std::unique_ptr<ProductionController> prodCtrl;
    std::unique_ptr<ReleaseController>    releaseCtrl;

    void SetUp() override {
        auto tmp = std::filesystem::temp_directory_path();
        samplePath = tmp / "test_samples.json";
        orderPath  = tmp / "test_orders.json";
        // 빈 JSON 초기화
        writeEmptyJson(samplePath);
        writeEmptyJson(orderPath);

        sampleRepo  = std::make_unique<JsonSampleRepository>(samplePath.string());
        orderRepo   = std::make_unique<JsonOrderRepository>(orderPath.string());
        sampleCtrl  = std::make_unique<SampleController>(*sampleRepo);
        orderCtrl   = std::make_unique<OrderController>(*orderRepo, *sampleRepo, prodQueue);
        prodCtrl    = std::make_unique<ProductionController>(prodQueue, *orderRepo, *sampleRepo);
        releaseCtrl = std::make_unique<ReleaseController>(*orderRepo, *sampleRepo);
    }

    void TearDown() override {
        std::filesystem::remove(samplePath);
        std::filesystem::remove(orderPath);
    }

    // 빈 JSON 배열 파일 생성
    void writeEmptyJson(const std::filesystem::path& path) { ... }

    // 기본 시료 등록 헬퍼
    void registerDefaultSample(const std::string& sampleId,
                               double avgProdTime, double yield, int stock) {
        sampleCtrl->registerSample(sampleId, "시료-" + sampleId,
                                   avgProdTime, yield, stock);
    }
};
```

---

### 시나리오 1 — 재고 충분 정상 경로

```
GIVEN: 시료 S-001 (stock=200, yield=0.92, avgProdTime=5.0)
WHEN:  주문 접수(qty=100) → 승인
THEN:  주문 상태 CONFIRMED
       시료 reservedQty == 100
WHEN:  출고
THEN:  주문 상태 RELEASE
       시료 stock == 100, reservedQty == 0
```

```
흐름:
1. sampleCtrl.registerSample("S-001", ..., stock=200)
2. orderId = orderCtrl.reserveOrder("S-001", "삼성전자", 100)
3. orderCtrl.approveOrder(orderId, now)  → CONFIRMED, reserveQty(100)
4. 상태 검증: orderRepo.findById(orderId).getStatus() == CONFIRMED
5. releaseCtrl.releaseOrder(orderId)
6. 상태 검증: orderRepo.findById(orderId).getStatus() == RELEASE
7. 재고 검증: sampleRepo.findById("S-001").getStock() == 100
              sampleRepo.findById("S-001").getReservedQty() == 0
```

---

### 시나리오 2 — 재고 부족 생산 경로

```
GIVEN: 시료 S-001 (stock=30, yield=0.9, avgProdTime=0.001)
       shortage=70, actualProd=87, completionTime≈startTime+5
WHEN:  주문 접수(qty=100) → 승인
THEN:  주문 상태 PRODUCING
       prodQueue.size() == 1
       시료 reservedQty == 100
WHEN:  tick(completionTime) 호출
THEN:  주문 상태 CONFIRMED
       시료 stock == 30+87=117, reservedQty == 100
WHEN:  출고
THEN:  주문 상태 RELEASE
       시료 stock == 17 (117-100), reservedQty == 0
```

```
흐름:
1. sampleCtrl.registerSample("S-001", ..., stock=30, avgProdTime=0.001)
2. orderId = orderCtrl.reserveOrder("S-001", "SK하이닉스", 100)
3. orderCtrl.approveOrder(orderId, now)  → PRODUCING
4. job = prodCtrl.peekNextJob()
5. prodCtrl.tick(job.getCompletionTime())  → CONFIRMED
6. 재고 검증: stock==117, reservedQty==100
7. releaseCtrl.releaseOrder(orderId)
8. 최종 검증: stock==17, reservedQty==0
```

---

### 시나리오 3 — 주문 거절

```
GIVEN: 시료 S-001 (stock=200)
WHEN:  주문 접수(qty=50) → 거절
THEN:  주문 상태 REJECTED
       prodQueue.empty() == true
       시료 stock 변동 없음 (200 유지)
```

```
흐름:
1. sampleCtrl.registerSample("S-001", ..., stock=200)
2. orderId = orderCtrl.reserveOrder("S-001", "LG전자", 50)
3. orderCtrl.rejectOrder(orderId)
4. 상태 검증: REJECTED
5. 재고 검증: stock==200
6. 큐 검증: prodQueue.empty() == true
```

---

### 시나리오 4 — FIFO 다중 생산

```
GIVEN: 시료 S-001 (stock=0, avgProdTime=0.001)
WHEN:  주문 2건 접수 후 각각 승인
THEN:  양쪽 모두 PRODUCING
       prodQueue.size() == 2
       첫 번째 처리 중, 두 번째 대기
WHEN:  tick(job1.completionTime) → 첫 번째 완료
THEN:  ORD-001 상태 CONFIRMED
       prodQueue.size() == 1 (ORD-002 대기)
WHEN:  tick(job2.completionTime) → 두 번째 완료
THEN:  ORD-002 상태 CONFIRMED
       prodQueue.empty()
```

```
흐름:
1. sampleCtrl.registerSample("S-001", ..., stock=0, avgProdTime=0.001)
2. orderId1 = orderCtrl.reserveOrder("S-001", "고객A", 50)
3. orderId2 = orderCtrl.reserveOrder("S-001", "고객B", 30)
4. orderCtrl.approveOrder(orderId1, now)  → PRODUCING, Queue[ORD-001]
5. orderCtrl.approveOrder(orderId2, now)  → PRODUCING, Queue[ORD-001, ORD-002]
6. job1 = prodQueue.front()
7. prodCtrl.tick(job1.getCompletionTime())  → ORD-001 CONFIRMED, Queue[ORD-002]
8. ORD-001 상태 검증: CONFIRMED
9. Queue 크기 검증: 1
10. job2 = prodQueue.front()
11. prodCtrl.tick(job2.getCompletionTime())  → ORD-002 CONFIRMED, Queue[]
12. ORD-002 상태 검증: CONFIRMED
13. Queue 비어있는지 검증
```

---

## vcxproj 추가 항목

### 항상 컴파일 (Debug + Release)

```xml
<ClCompile Include="src\view\MainView.cpp" />
<ClCompile Include="src\view\ProductionView.cpp" />
```

### Release 전용 (프로그램 진입점)

```xml
<ClCompile Include="src\main.cpp" Condition="'$(Configuration)'=='Release'" />
```

※ main.cpp는 이미 Release 전용으로 등록되어 있음 — 확인만 필요

### Debug 전용 (통합 테스트)

```xml
<ClCompile Include="test\test_integration.cpp" Condition="'$(Configuration)'=='Debug'" />
```

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| Debug 빌드 | 오류 0, 통합 테스트 4개 통과 |
| Release 빌드 | 오류 0, 프로그램 정상 실행 |
| 메뉴 | 0~6번 선택 모두 정상 동작 |
| 생산 자동 완료 | 메인 루프에서 tick() 호출로 자동 전환 |
| 데이터 영속성 | 재실행 후 data/ 파일에서 복원 |
| 임시 파일 격리 | 테스트 종료 후 temp 파일 삭제 확인 |
| 누적 테스트 수 | Phase 8까지 누적 + 4개 = 97개 이상 통과 |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| `ProductionView` 타입 불일치 | `showProductionQueue`를 `vector` 인자로 변경해야 컴파일 통과 |
| `MonitoringController` 생성자 | Phase 9에서 추가된 `prodQueue` 3번째 인자를 main.cpp에도 전달 필수 |
| `approveOrder(orderId, nowSec)` | main.cpp에서 `std::time(nullptr)` 캐스팅하여 전달 |
| 임시 파일 경로 | `std::filesystem`(C++17) 사용, `<filesystem>` 헤더 추가 필요 |
| 빈 JSON 초기화 | `{"samples":[]}` / `{"orders":[],"nextSequence":1}` 형식 확인 후 맞춰 작성 |
| FIFO 순서 검증 | 시나리오 4에서 `prodQueue.front().getOrderId()`로 순서 명시적 검증 권장 |
| 생산 완료 시간 주입 | `getCompletionTime()` 사용 — 테스트 시 시간 의존성 제거 |