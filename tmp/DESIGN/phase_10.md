# Phase 10 상세 검증 계획 — PRD 대조 및 품질 점검

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_09.md](./phase_09.md)
> 대상 Phase: 10 (최종 검증 — 신규 코드 없음, 점검·수정만)
> 총 테스트: 93개 현재 (PLAN.md 기준 73개 이상 요건 충족)

---

## 개요

Phase 10은 Phase 1~9에서 구현한 전체 기능을 **PRD 명세와 대조**하고  
**코드 품질 기준**을 충족하는지 점검하는 최종 검증 단계다.  
새 기능을 추가하지 않으며, 발견된 갭과 품질 위반은 그 자리에서 수정한다.

**검증 방식**

```
1. Debug 빌드 → 자동 테스트 93개 전량 Green 확인
2. 코드 품질 체크리스트 항목별 grep / 코드 리뷰
3. Release 빌드 → 수동 시나리오 실행 (콘솔 입력)
4. 갭 발견 시 → 즉시 수정 후 재검증
```

---

## 현재 구현 파일 현황

| 레이어 | 파일 | 상태 |
|--------|------|------|
| Model | Sample, Order, ProductionJob, OrderStatus, MonitoringSummary | ✅ 완성 |
| Repository | JsonSampleRepository, JsonOrderRepository, JsonProductionJobRepository | ✅ 완성 |
| Controller | SampleController, OrderController, ProductionController, MonitoringController, ReleaseController | ✅ 완성 |
| View | SampleView, OrderView, ProductionView, MonitorView, MainView | ✅ 완성 |
| 진입점 | main.cpp | ✅ 완성 |
| 데이터 | data/samples.json, data/orders.json, data/production_jobs.json | ✅ 완성 |
| 테스트 | test_*.cpp 16개 파일, 총 93개 TC | ✅ 완성 |

---

## PRD 기능 대조 체크리스트

각 항목에 대해 **자동 테스트 파일**, **확인 포인트**, **수동 확인 절차**를 명시한다.

---

### 1. 시료 등록 / 조회 / 검색

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_sample_controller.cpp` | 7 | 등록·중복검사·수율범위·조회·검색 |
| `test_json_sample_repository.cpp` | 6 | JSON 파일 CRUD·영속성 |
| `test_sample_view.cpp` | 2 | 목록 출력·빈목록 안내 |

**확인 포인트**
- `SampleController::registerSample` — 수율 0 초과 1 이하, 평균생산시간 양수 검증 포함 여부
- `searchByName` — 부분 일치(keyword가 name에 포함) 동작 여부
- `data/samples.json` 재시작 후 데이터 유지 여부

**수동 확인 (Release 빌드)**
```
메뉴 1 → 1 (시료 등록): S-001 / 알파칩 / 5.0 / 0.92 / 500 입력
메뉴 1 → 2 (전체 조회): S-001 목록 출력 확인
메뉴 1 → 3 (이름 검색): "알파" 입력 → S-001 검색됨 확인
프로그램 재시작 → 메뉴 1 → 2: 데이터 유지 확인
```

---

### 2. 주문 접수 (RESERVED, 주문번호 형식)

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_order.cpp` | 7 | 생성자·상태전이·주문번호 형식 |
| `test_order_controller.cpp` | 12 | reserveOrder 정상/예외 경로 |
| `test_json_order_repository.cpp` | 5 | 저장·조회·시퀀스 |

**확인 포인트**
- 주문번호 정규식: `ORD-\d{8}-\d{4}` 형식 준수
- `getNextSequence` — 기존 ID에서 최대 시퀀스 추출 방식으로 변경됨 (리팩토링 [6])
- 존재하지 않는 시료 ID / 수량 0 이하 / 빈 고객명 → 예외 발생 확인

**수동 확인**
```
메뉴 2 → 시료ID: S-001 / 고객명: 삼성전자 / 수량: 100
→ "[주문 접수 완료] ORD-YYYYMMDD-NNNN" 출력 확인
메뉴 2 → 존재하지 않는 시료ID 입력 → "[오류]" 출력 후 메뉴 복귀 확인
```

---

### 3. 주문 승인 → 재고 분기 (CONFIRMED / PRODUCING)

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_order_controller.cpp` | 12 | 재고 충분/부족/경계·reservedQty 추적 |
| `test_stock_aware_approve.cpp` | 5 | 생산 진척 반영 실효재고 기반 승인 |
| `test_production_stock_flow.cpp` | 7 | 생산→완료→출고 전체 흐름 |
| `test_integration.cpp` | 4 | 종단간 시나리오 |

**확인 포인트**

| 조건 | 기대 결과 |
|------|-----------|
| effectiveStock ≥ qty | CONFIRMED, reservedQty += qty |
| effectiveStock < qty | PRODUCING, prodQueue에 job 추가, reservedQty += qty |
| RESERVED 아닌 주문 승인 | `std::logic_error` 발생 |

- `effectiveStock = stock + getInProgressUnits(queue, nowSec)`
- `availableStock = effectiveStock - reservedQty`
- `shortage = qty - max(0, availableStock)`

**수동 확인**
```
재고 충분 경로: stock=500, 주문qty=100 → 승인 → CONFIRMED
재고 부족 경로: stock=30, 주문qty=100 → 승인 → PRODUCING + 생산 현황 확인
```

---

### 4. 주문 거절 → REJECTED

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_order_controller.cpp` | 1(TC5) | rejectOrder → REJECTED 전환 |
| `test_integration.cpp` | 1(TC3) | 거절 후 stock 불변·큐 비어있음 |

**확인 포인트**
- REJECTED 주문은 `Order::isMonitored()` → false → 모니터링 집계에서 제외
- 거절 후 재고·예약수량 변동 없음

**수동 확인**
```
메뉴 3 → RESERVED 주문 선택 → 2(거절) → "[주문 거절 완료]" 확인
메뉴 5 (모니터링) → 거절 주문이 집계에서 0건으로 표시되는지 확인
```

---

### 5. 생산라인 FIFO 처리

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_production_controller.cpp` | 6 | FIFO 순서·tick 완료·재고 update |
| `test_integration.cpp` | 1(TC4) | 다중 생산 FIFO 순서 보장 |

**확인 포인트**
- `std::queue<ProductionJob>` — 단일 라인, 한 번에 하나만 처리
- `tick(nowSec)` — 완료 조건: `nowSec >= completionTime`
- 완료 시: `sample.addStock(actualProd)` + `order.completeProduction()` → CONFIRMED

**수동 확인**
```
stock=0 시료로 주문 2건 승인 → 메뉴 4 (생산 현황) → 2건 대기 목록 표시
첫 번째 완료 후 → 두 번째 자동 시작 확인
```

---

### 6. 생산량 공식 정확성

**공식**: `actualProd = ceil(shortage / (yield × 0.9))`

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_production_job.cpp` | 6 | 공식 2케이스·총생산시간·시작직후 진행량 |
| `test_production_stock_flow.cpp` | 7 | shortage 계산 + actualProd 일치 확인 |

**검증 케이스**

| shortage | yield | 기대 actualProd | 계산 |
|----------|-------|-----------------|------|
| 170 | 0.92 | 206 | ceil(170 / 0.828) = ceil(205.31) = 206 |
| 150 | 0.88 | 190 | ceil(150 / 0.792) = ceil(189.39) = 190 |
| 70 | 0.9 | 87 | ceil(70 / 0.81) = ceil(86.42) = 87 |

---

### 7. 모니터링 (상태 집계·재고 상태·생산 진행도)

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_monitoring_controller.cpp` | 7 | 집계·StockStatus·생산 진척 |

**확인 포인트**

| 기능 | 구현 위치 | 검증 방법 |
|------|-----------|-----------|
| 주문 상태별 집계 (REJECTED 제외) | `MonitoringController::getOrderSummary` | TC1 |
| 재고 여유/부족/고갈 | `MonitoringController::getStockStatus` | TC2~4 |
| 생산 진행도 n/Total개 (X%) | `MonitoringController::getProductionProgress` | TC6~7 |
| 완료 예정 시각 YYYY-MM-DD HH:MM | `MonitorView::showProductionProgress` | 수동 확인 |

**수동 확인**
```
메뉴 5 (모니터링) 실행
→ 주문 현황: RESERVED/PRODUCING/CONFIRMED/RELEASE 건수 표시 확인
→ 생산 진행: "43/87개 (49%) | 완료 예정: 2026-06-12 15:30" 형식 확인
→ 재고 상태: [여유]/[부족]/[고갈] 태그 확인
```

---

### 8. 출고 처리 (CONFIRMED → RELEASE, 재고 차감)

**자동 테스트**

| 파일 | TC 수 | 핵심 검증 내용 |
|------|-------|----------------|
| `test_release_controller.cpp` | 5 | RELEASE 전환·재고 차감·예외 경로 |
| `test_integration.cpp` | 2(TC1,TC2) | 전체 흐름에서 출고 후 stock/reservedQty 검증 |

**확인 포인트**
- `sample.reduceStock(qty)` + `sample.releaseQty(qty)` 동시 호출
- sample 먼저 저장 후 order 저장 (데이터 정합성 — 리팩토링 [4] 적용)
- CONFIRMED 아닌 주문 → `std::logic_error`
- 재고 부족 → `std::runtime_error`

**수동 확인**
```
CONFIRMED 주문에 대해 메뉴 6 → 출고
→ "[출고 완료] ORD-..." 출력 확인
→ 메뉴 5 (모니터링) → RELEASE 건수 +1, stock 차감 확인
```

---

### 9. 데이터 영속성

**영속 파일 3종**

| 파일 | 담당 Repository | 재시작 복원 대상 |
|------|-----------------|-----------------|
| `data/samples.json` | JsonSampleRepository | 시료 전체 (stock, reservedQty 포함) |
| `data/orders.json` | JsonOrderRepository | 주문 전체 (status 포함) |
| `data/production_jobs.json` | JsonProductionJobRepository | 생산 큐 (startTime 포함) |

**확인 포인트**
- `JsonProductionJobRepository` — 매 루프 종료 시 저장, 시작 시 복원
- `ProductionJob` 복원 생성자 — `startTime`을 현재 시각이 아닌 저장된 값으로 주입
- 재시작 후 PRODUCING 주문이 생산 현황에 정상 표시되어야 함

**수동 확인 절차 (재시작 영속성 테스트)**
```
1. stock=30인 시료로 qty=100 주문 접수 및 승인 → PRODUCING 확인
2. 메뉴 4 (생산 현황) → 진행 상태 및 완료 예정 시각 기록
3. 프로그램 종료 (메뉴 0)
4. 프로그램 재시작
5. 메뉴 4 → 이전과 동일한 job이 남아있고, 진행률이 경과 시간에 맞게 업데이트됨 확인
6. 메뉴 5 (모니터링) → PRODUCING 1건 집계 확인
```

---

## 코드 품질 체크리스트

### Q1. Controller에 출력 코드 없음

**검증 방법**: grep으로 Controller 디렉터리 내 `std::cout` 검색

```
검색 결과: 0건 → 통과 ✅
(리팩토링 Phase 완료 후 확인)
```

**현재 상태**: `src/controller/*.cpp` 전체 — `std::cout` 0건 확인

---

### Q2. View에 비즈니스 로직 없음

**검증 방법**: View 파일에서 계산 관련 패턴 확인

```
확인 대상: ceil / getStock() 비교 / reservedQty 연산 / shortage 계산
검색 결과: getQuantity()/getStock() 단순 출력만 존재, 연산 없음 → 통과 ✅
```

---

### Q3. 테스트 수 73개 이상

**현재 테스트 수**

| 파일 | TC 수 |
|------|-------|
| test_phase0_smoke.cpp | 2 |
| test_sample.cpp | 11 |
| test_order.cpp | 7 |
| test_production_job.cpp | 6 |
| test_mock_compile.cpp | 1 |
| test_json_sample_repository.cpp | 6 |
| test_json_order_repository.cpp | 5 |
| test_sample_controller.cpp | 7 |
| test_sample_view.cpp | 2 |
| test_order_controller.cpp | 12 |
| test_production_controller.cpp | 6 |
| test_monitoring_controller.cpp | 7 |
| test_release_controller.cpp | 5 |
| test_production_stock_flow.cpp | 7 |
| test_stock_aware_approve.cpp | 5 |
| test_integration.cpp | 4 |
| **합계** | **93** |

기준 73개 대비 **+20개** — 통과 ✅

---

### Q4. 임시 파일 격리

**검증 방법**: 통합 테스트 `TearDown()` 확인

```cpp
// test_integration.cpp
void TearDown() override {
    std::filesystem::remove(samplePath);  // tmp/it_samples.json
    std::filesystem::remove(orderPath);   // tmp/it_orders.json
}
```

- 실제 `data/` 파일을 건드리지 않음 ✅
- 테스트 종료 후 임시 파일 삭제됨 ✅

---

### Q5. 예외 처리 경로 확인

**확인 포인트**

| 입력 오류 | 처리 위치 | 기대 동작 |
|-----------|-----------|-----------|
| 없는 시료 ID로 주문 | OrderController::reserveOrder | runtime_error → mainView.showError |
| 음수/0 수량 | OrderController::reserveOrder | invalid_argument → mainView.showError |
| RESERVED 아닌 주문 승인 | OrderController::approveOrder | logic_error → mainView.showError |
| 재고 부족 출고 | ReleaseController::releaseOrder | runtime_error → mainView.showError |
| 수율 범위 외 시료 등록 | SampleController::registerSample | invalid_argument → mainView.showError |
| 잘못된 메뉴 입력 | main.cpp 메인 루프 default | showError("없는 메뉴입니다.") |

**수동 확인**: 각 오류 경로를 직접 입력해 `[오류]` 출력 후 메뉴 복귀하는지 확인

---

### Q6. 코딩 컨벤션

**확인 포인트**

| 항목 | 기준 | 확인 방법 |
|------|------|-----------|
| 클래스명 | PascalCase | 코드 리뷰 |
| 멤버변수 | `m_` prefix + camelCase | 코드 리뷰 |
| 함수명 | camelCase | 코드 리뷰 |
| enum | UPPER_SNAKE_CASE | OrderStatus 확인 |
| 주석 | Why만 작성, What 금지 | 코드 리뷰 |
| 들여쓰기 | 탭 | 파일 바이트 확인 |
| 인코딩 | UTF-8 with BOM | BOM (EF BB BF) 확인 |

---

## 수동 테스트 시나리오 (Release 빌드)

### 시나리오 A — 전체 정상 흐름

```
1. 시료 등록: S-001 / 알파칩 / avgProdTime=5.0 / yield=0.92 / stock=500
2. 주문 접수: S-001 / 삼성전자 / qty=100
3. 주문 승인 → CONFIRMED (재고 충분)
4. 출고 처리 → RELEASE
5. 모니터링 → RELEASE 1건, stock=400 확인
6. 종료 후 재시작 → 데이터 유지 확인
```

### 시나리오 B — 생산 경로

```
1. 시료 등록: S-002 / 베타칩 / avgProdTime=1.0 / yield=0.9 / stock=30
2. 주문 접수: S-002 / SK하이닉스 / qty=100
3. 주문 승인 → PRODUCING (재고 부족)
4. 메뉴 4 (생산 현황) → "0/87개 (0%) | 완료 예정: YYYY-MM-DD HH:MM" 확인
5. 메뉴 5 (모니터링) → 생산 진행도 확인
6. 종료 후 재시작 → 메뉴 4에서 job 복원 확인
7. 생산 완료 후 → CONFIRMED → 출고
```

### 시나리오 C — 오류 경로

```
1. 없는 시료 ID로 주문 → [오류] 출력 확인
2. qty=0으로 주문 → [오류] 출력 확인
3. PRODUCING 주문 직접 승인 시도 → [오류] 출력 확인
4. 잘못된 메뉴 번호(9) → [오류] 없는 메뉴입니다. 확인
```

---

## 알려진 갭 및 검토 항목

| # | 항목 | 상태 | 조치 |
|---|------|------|------|
| 1 | `data/production_jobs.json` 재시작 복원 | ✅ 구현됨 (Phase 10 이전에 수정) | 수동 시나리오 B로 확인 |
| 2 | 생산 현황 진행도·포맷 시각 | ✅ 구현됨 | 수동 시나리오 B로 확인 |
| 3 | 모니터링 진행도·포맷 시각 | ✅ 구현됨 | 수동 시나리오 B로 확인 |
| 4 | `showCompleteSuccess` dead code | ✅ 제거됨 (리팩토링) | — |
| 5 | `getNextSequence` 중복 시퀀스 취약점 | ✅ 수정됨 (리팩토링 [6]) | — |
| 6 | PRODUCING 주문 재승인 방지 | ✅ 동작 확인 | approveOrder 시 RESERVED 체크 |
| 7 | 생산 완료 후 queue에서 pop되는지 | 자동 TC + 수동 B로 확인 필요 | — |

---

## 완료 기준

| 조건 | 기준 | 현재 상태 |
|------|------|-----------|
| Debug 빌드 | 오류 0, 경고 최소화 | 확인 필요 |
| 자동 테스트 | 93개 전량 Green | 확인 필요 |
| PRD 체크리스트 9항목 | 전량 충족 | 수동 확인 필요 |
| 코드 품질 6항목 | 전량 충족 | Q1~Q2 ✅, Q3 ✅, Q4 ✅ |
| Release 빌드 | 오류 0, 시나리오 A/B/C 정상 동작 | 수동 확인 필요 |
| 데이터 영속성 | 재시작 후 samples·orders·jobs 복원 | 수동 확인 필요 |

---

## 검증 순서 (추천)

```
Step 1. Debug 빌드 → 테스트 전량 실행 → 93개 Green 확인
Step 2. Q1~Q6 코드 품질 체크리스트 순서대로 점검
Step 3. Release 빌드 → 시나리오 A (정상 흐름) 실행
Step 4. Release 빌드 → 시나리오 B (생산 경로 + 재시작 복원) 실행
Step 5. Release 빌드 → 시나리오 C (오류 경로) 실행
Step 6. 발견된 갭 수정 → Step 1 반복
Step 7. 모든 항목 Green → Phase 10 완료
```