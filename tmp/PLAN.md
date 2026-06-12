# PLAN.md — 반도체 시료 생산주문관리 시스템 개발 계획

> 참조: [PRD.md](../PRD.md) | [CLAUDE.md](../CLAUDE.md)
> 개발 방식: TDD (GoogleTest + GMock) | 환경: Visual Studio 2022, C++17

---

## 개발 흐름 요약

```
test-writer가 테스트 작성
    → code-writer가 구현
        → 사용자가 테스트 실행 (Green 확인)
            → (필요 시) refactoring
```

---

## 전체 Phase 일람

| Phase | 이름 | 핵심 작업 | 테스트 수 | 상태 |
|-------|------|----------|----------|------|
| 0 | 프로젝트 초기 설정 | VS 솔루션 구성, GoogleTest 연동 | 2 | ⬜ |
| 1-1 | Sample 모델 | 시료 도메인 객체, 재고 증감 | 6 | ⬜ |
| 1-2 | Order 모델 | 주문 도메인 객체, 상태 전이 | 6 | ⬜ |
| 1-3 | ProductionJob 모델 | 생산량 계산 공식 | 6 | ⬜ |
| 수정사항 | reservedSTock 추가 | 예약 수량 filed 추가 | 6 | ⬜ |
| 2-1 | Repository 인터페이스 | Mock 클래스 정의 | 1 | ⬜ |
| 2-2 | JsonSampleRepository | 시료 JSON 파일 CRUD | 6 | ⬜ |
| 2-3 | JsonOrderRepository | 주문 JSON 파일 CRUD | 5 | ⬜ |
| 3 | 시료 관리 | SampleController + SampleView | 9 | ⬜ |
| 4 | 주문 접수 | OrderController::reserve | 5 | ⬜ |
| 5 | 주문 승인/거절 | OrderController::approve/reject | 7 | ⬜ |
| 6 | 생산라인 | ProductionController, FIFO 큐 | 6 | ⬜ |
| 7 | 모니터링 | MonitoringController | 5 | ⬜ |
| 8 | 출고 처리 | ReleaseController | 5 | ⬜ |
| 9 | 통합 | main.cpp 조립 + 종단간 시나리오 | 4 | ⬜ |
| 10 | 최종 검증 | PRD 대조, 품질 점검 | - | ⬜ |
| | | **합계** | **73** | |

---

## 각 Phase 상세

---

### Phase 0. 프로젝트 초기 설정

**목표**: 빌드 환경을 잡고 "Hello Test"가 통과하는 상태까지 만든다.

**할 일**
- Visual Studio 솔루션 생성 (`SemiProdiction` 앱 + `SemiProdictionTest` 테스트 프로젝트)
- NuGet 또는 vcpkg로 GoogleTest / GMock 설치
- 폴더 구조 생성 (`src/model`, `src/controller`, `src/view`, `src/repository`, `data/`)
- `data/samples.json`, `data/orders.json` 빈 파일 생성

**테스트**
- 빌드가 성공하는지 확인하는 Smoke Test
- GMock이 정상 동작하는지 확인하는 Mock Test

**완료 기준**: 빌드 오류 0, 테스트 2개 통과

---

### Phase 1-1. Sample 모델

**목표**: 시료(Sample)의 데이터 구조와 재고 관련 동작을 구현한다.

**할 일**
- `OrderStatus` 열거형 정의 (RESERVED / REJECTED / PRODUCING / CONFIRMED / RELEASE)
- 상태값 ↔ 문자열 변환 함수 구현
- `Sample` 클래스 구현 (시료 ID, 이름, 평균 생산시간, 수율, 재고)
- 재고 추가 / 재고 감소 / 재고 충분 여부 확인 메서드

**테스트**
- 생성자가 값을 올바르게 저장하는지
- 재고 추가가 정확히 반영되는지
- 재고 감소가 정확히 반영되는지
- 재고보다 많이 감소 시 예외가 발생하는지
- 재고 충분 여부 판단이 맞는지 (경계값 포함)
- 상태 문자열 변환이 정확한지

**완료 기준**: 테스트 6개 통과

---

### Phase 1-2. Order 모델

**목표**: 주문(Order)의 데이터 구조와 상태 관리를 구현한다.

**할 일**
- `Order` 클래스 구현 (주문번호, 시료 ID, 고객명, 수량, 상태, 생성일시)
- 주문번호 자동 생성 (`ORD-YYYYMMDD-NNNN` 형식)
- 모니터링 포함 여부 판단 (REJECTED이면 제외)

**테스트**
- 생성 직후 상태가 RESERVED인지
- 상태를 변경할 수 있는지
- REJECTED 주문이 모니터링 대상에서 제외되는지
- 나머지 상태는 모니터링 대상인지
- 주문번호 형식이 `ORD-YYYYMMDD-NNNN`인지
- 음수 수량으로 생성 시 예외가 발생하는지

**완료 기준**: 테스트 6개 통과

---

### Phase 1-3. ProductionJob 모델

**목표**: 생산 작업의 계산 로직을 구현한다. 특히 생산량 공식이 정확해야 한다.

**할 일**
- `ProductionJob` 클래스 구현 (주문 ID, 시료 ID, 부족분, 실생산량, 총 생산시간, 시작 시각)
- **핵심 공식**: `실생산량 = ceil(부족분 / (수율 × 0.9))`
- 생산 완료 여부 판단 (현재 시각 기준)
- 현재까지 생산된 수량 계산 (경과 시간 비례)
- 완료 예정 시각 계산

**테스트**
- 공식 검증 케이스 1: 부족분 170, 수율 0.92 → 실생산량 206
- 공식 검증 케이스 2: 부족분 150, 수율 0.88 → 실생산량 190
- 총 생산시간 = 평균 생산시간 × 실생산량
- 생성 직후에는 완료되지 않은 상태인지
- 완료 예정 시각이 시작 시각보다 큰지
- 시작 직후 현재 생산량이 0인지

**완료 기준**: 테스트 6개 통과, **공식 정확성 필수 확인**

---

### Phase 2-1. Repository 인터페이스 + Mock

**목표**: Controller가 파일 없이 테스트될 수 있도록 인터페이스와 Mock을 정의한다.

**할 일**
- `ISampleRepository` 인터페이스 정의 (저장 / 수정 / ID 조회 / 전체 조회 / 이름 검색 / 존재 여부)
- `IOrderRepository` 인터페이스 정의 (저장 / 수정 / ID 조회 / 전체 조회 / 상태별 조회 / 시퀀스 채번)
- `MockSampleRepository`, `MockOrderRepository` Mock 클래스 작성

**테스트**
- Mock이 컴파일되고 EXPECT_CALL이 정상 동작하는지 확인

**완료 기준**: Mock 컴파일 성공, 이후 모든 Controller 테스트에서 재사용 가능

---

### Phase 2-2. JsonSampleRepository

**목표**: 시료 데이터를 `data/samples.json`에 읽고 쓰는 실제 구현체를 만든다.

**할 일**
- `nlohmann/json` (헤더 온리) 또는 `rapidjson` 라이브러리 도입
- `ISampleRepository` 구현 — 저장 / 수정 / 조회 / 검색 모두 JSON 파일에 반영
- 이름 검색은 **부분 일치**로 동작

**테스트** (임시 파일로 격리, 테스트 종료 시 삭제)
- 저장 후 동일 ID로 조회하면 같은 데이터가 나오는지
- 없는 ID 조회 시 예외가 발생하는지
- 저장 2개 후 전체 조회 시 2개가 나오는지
- 수정 후 재조회 시 변경된 값이 나오는지
- 이름 부분 일치 검색이 동작하는지
- 새 인스턴스로 파일을 다시 읽었을 때 데이터가 유지되는지 (영속성)

**완료 기준**: 테스트 6개 통과, 실제 `data/` 파일 오염 없음

---

### Phase 2-3. JsonOrderRepository

**목표**: 주문 데이터를 `data/orders.json`에 읽고 쓰는 실제 구현체를 만든다.

**할 일**
- `IOrderRepository` 구현 — 저장 / 수정 / 조회 / 상태별 조회 / 시퀀스 채번
- 시퀀스는 파일에서 관리하여 재실행 후에도 중복 없이 증가

**테스트** (임시 파일로 격리)
- 저장 후 ID 조회
- 상태별 조회 (RESERVED만 조회 시 CONFIRMED는 제외되는지)
- 시퀀스가 호출마다 1씩 증가하는지
- 상태 수정 후 재조회 시 반영되는지
- 재실행 후 데이터 유지 (영속성)

**완료 기준**: 테스트 5개 통과

---

### Phase 3. 시료 관리 (SampleController + SampleView)

**목표**: 시료 등록 / 전체 조회 / 이름 검색 기능을 완성한다.

**할 일**
- `SampleController` — 시료 등록 (중복 검사, 입력값 검증 포함), 전체 조회, 이름 검색, ID 조회
- `SampleView` — 시료 목록 테이블 출력, 빈 목록 안내 문구 출력 (비즈니스 로직 없음)

**테스트** (Controller는 MockRepository 사용)
- 정상 등록 시 Repository의 save가 호출되는지
- 이미 있는 ID로 등록 시 예외가 발생하는지
- 수율이 범위(0 초과 ~ 1 이하)를 벗어나면 예외가 발생하는지
- 평균 생산시간이 음수이면 예외가 발생하는지
- 전체 조회가 Repository 결과를 그대로 반환하는지
- 이름 검색이 Repository에 올바르게 위임되는지
- 검색 결과가 없을 때 빈 목록을 반환하는지
- View: 목록 출력 시 ID, 이름, 재고가 포함되는지
- View: 빈 목록일 때 안내 문구가 출력되는지

**완료 기준**: 테스트 9개 통과

---

### Phase 4. 주문 접수 (OrderController::reserve)

**목표**: 고객 주문을 RESERVED 상태로 생성한다.

**할 일**
- `OrderController::reserveOrder` 구현
- 시료 존재 여부 확인 → 없으면 예외
- 수량 / 고객명 입력값 검증
- 주문번호 자동 생성 (`ORD-YYYYMMDD-NNNN`)

**테스트** (MockRepository 사용)
- 정상 예약 시 RESERVED 상태로 생성되는지
- 생성된 주문번호 형식이 맞는지
- 존재하지 않는 시료 ID로 주문 시 예외가 발생하는지
- 수량이 0 이하이면 예외가 발생하는지
- 고객명이 비어있으면 예외가 발생하는지

**완료 기준**: 테스트 5개 통과

---

### Phase 5. 주문 승인/거절 (OrderController::approve / reject)

**목표**: RESERVED 주문을 승인하거나 거절한다. 승인 시 재고 상황에 따라 자동 분기한다.

**할 일**
- `OrderController::approveOrder` — 재고 충분이면 CONFIRMED, 부족이면 생산라인 등록 후 PRODUCING
- `OrderController::rejectOrder` — 즉시 REJECTED
- `OrderController::getReservedOrders` — RESERVED 목록 조회

**테스트** (MockRepository + MockProductionQueue 사용)
- 재고 충분(200 재고, 100 주문) → CONFIRMED 전환
- 재고 부족(30 재고, 200 주문) → PRODUCING 전환 + 생산라인 등록
- 재고 = 주문 수량 (경계) → CONFIRMED 전환
- RESERVED가 아닌 주문 승인 시 예외가 발생하는지
- 거절 시 REJECTED로 전환되는지
- RESERVED 목록 조회가 올바르게 동작하는지
- 생산라인 등록 시 실생산량 공식(206)이 올바르게 적용되는지

**완료 기준**: 테스트 7개 통과

---

### Phase 6. 생산라인 (ProductionController)

**목표**: FIFO 방식으로 생산 작업을 순서대로 처리한다.

**할 일**
- `ProductionController` — FIFO 큐 관리, 현재 작업 처리, 완료 감지
- `tick()` — 메인 루프마다 호출하여 완료된 작업을 CONFIRMED으로 전환
- 완료 시 재고 자동 추가 + 다음 작업 자동 시작

**테스트**
- 시작 시 큐가 비어 있고 idle 상태인지
- 작업 추가 시 즉시 처리 시작되는지 (idle일 때)
- 3개 추가 시 처리 순서가 FIFO(ORD-001 → ORD-002 → ORD-003)인지
- 완료 시 CONFIRMED 전환 + 재고 update가 호출되는지
- 진행률이 0~100% 범위 안에 있는지
- 빈 큐에서 tick() 호출 시 아무 일도 없는지

**완료 기준**: 테스트 6개 통과

---

### Phase 7. 모니터링 (MonitoringController)

**목표**: 시스템 현황(주문 건수, 재고 상태)을 한눈에 확인할 수 있게 한다.

**할 일**
- 상태별 주문 건수 집계 (REJECTED 제외)
- 시료별 재고 상태 산출: **여유** (재고 ≥ 주문량) / **부족** (0 < 재고 < 주문량) / **고갈** (재고 = 0)

**테스트**
- 6건 중 REJECTED 1건을 제외한 5건이 올바르게 집계되는지
- 재고 = 주문량일 때 "여유"인지 (경계값)
- 재고 < 주문량일 때 "부족"인지
- 재고 = 0일 때 "고갈"인지
- 시료 전체에 대해 재고 현황 목록이 반환되는지

**완료 기준**: 테스트 5개 통과

---

### Phase 8. 출고 처리 (ReleaseController)

**목표**: CONFIRMED 상태 주문을 출고하여 RELEASE로 전환한다.

**할 일**
- `ReleaseController::getConfirmedOrders` — 출고 가능한 주문 목록 조회
- `ReleaseController::releaseOrder` — 출고 실행: RELEASE 전환 + 재고 차감

**테스트**
- 출고 후 주문 상태가 RELEASE로 바뀌는지
- 출고 후 재고가 정확히 차감되는지 (300 재고, 150 출고 → 150 잔여)
- CONFIRMED가 아닌 주문 출고 시 예외가 발생하는지
- CONFIRMED 목록만 조회되는지
- 재고보다 많은 수량 출고 시 예외가 발생하는지

**완료 기준**: 테스트 5개 통과

---

### Phase 9. 통합 및 메인 메뉴 조립

**목표**: 모든 Controller를 `main.cpp`에 연결하고 전체 흐름을 검증한다.

**할 일**
- `main.cpp` — Repository 생성 → Controller 조립 → 메인 루프 (메뉴 라우팅)
- 메인 루프마다 `prodCtrl.tick()` 호출하여 생산 완료 자동 감지
- 각 메뉴 선택 시 해당 View로 연결

**통합 테스트 시나리오** (실제 임시 JSON 파일 사용)

| 시나리오 | 흐름 |
|----------|------|
| 1. 재고 충분 정상 경로 | 시료 등록 → 주문 → 승인(CONFIRMED) → 출고(RELEASE), 재고 차감 확인 |
| 2. 재고 부족 생산 경로 | 시료 등록(소량) → 주문 → 승인(PRODUCING) → 생산 완료(CONFIRMED) → 출고 |
| 3. 주문 거절 | 주문 → 거절(REJECTED) → 모니터링 집계에서 0건 확인 |
| 4. FIFO 다중 생산 | 재고 0 → 주문 2건 승인 → 첫 번째가 처리 중, 두 번째가 대기 확인 |

**완료 기준**: 통합 시나리오 4개 통과, 콘솔 메뉴 정상 동작

---

### Phase 10. 최종 검증

**목표**: PRD 기능 명세와 코드 품질을 최종 점검한다.

**PRD 기능 대조 체크리스트**
- [ ] 시료 등록 / 조회 / 검색
- [ ] 주문 접수 (RESERVED, 주문번호 형식)
- [ ] 주문 승인 → 재고 충분: CONFIRMED / 재고 부족: PRODUCING
- [ ] 주문 거절 → REJECTED
- [ ] 생산라인 FIFO 처리
- [ ] 생산량 공식 `ceil(부족분 / (수율 × 0.9))` 정확성
- [ ] 모니터링 (상태별 집계, 재고 여유/부족/고갈, REJECTED 제외)
- [ ] 출고 처리 (CONFIRMED → RELEASE, 재고 차감)
- [ ] 데이터 영속성 (재실행 후 데이터 유지)

**코드 품질 체크리스트**
- [ ] Controller에 `std::cout` 없음 (View가 담당)
- [ ] View에 비즈니스 계산 없음 (Controller가 담당)
- [ ] 테스트 총 73개 이상 통과
- [ ] 임시 파일 격리 — 실제 `data/` 파일 오염 없음
