# S-Semi 반도체 시료 생산주문관리 시스템

반도체 회사 **S-Semi**의 시료(Sample) 주문·생산·출고 흐름을 관리하는 C++ 콘솔 애플리케이션입니다.

---

## 개발 환경

| 항목 | 내용 |
|------|------|
| 언어 | C++17 |
| IDE | Visual Studio 2022 |
| 빌드 | MSBuild (Debug / Release) |
| 테스트 | GoogleTest + GMock (NuGet: gmock 1.11.0) |
| 데이터 저장 | JSON 파일 (`data/`) |
| 실행 환경 | Windows 콘솔 |

---

## 프로젝트 구조

```
SemiProdiction/               ← 솔루션 루트
├── SemiProdiction.slnx       ← Visual Studio 솔루션
├── data/
│   ├── samples.json          ← 시료 데이터 (영속)
│   ├── orders.json           ← 주문 데이터 (영속)
│   └── production_jobs.json  ← 생산 큐 (영속 — 재시작 시 복원)
└── SemiProdiction/           ← 프로젝트 디렉터리
    └── src/
        ├── main.cpp          ← 진입점 (Application::run() 호출만)
        ├── app/
        │   ├── Application.h
        │   └── Application.cpp   ← 메인 루프 + 메뉴 핸들러 6개
        ├── model/
        │   ├── Sample            ← 시료 (ID·이름·수율·생산시간·재고·예약수량)
        │   ├── Order             ← 주문 (번호·상태·고객·수량)
        │   ├── ProductionJob     ← 생산 작업 (부족분·실 생산량·시작시각)
        │   ├── OrderStatus       ← 주문 상태 enum
        │   └── MonitoringSummary ← 모니터링용 집계 구조체
        ├── controller/
        │   ├── SampleController      ← 시료 등록·조회·검색
        │   ├── OrderController       ← 주문 접수·승인·거절
        │   ├── ProductionController  ← 생산라인 tick·진행 조회
        │   ├── MonitoringController  ← 집계·재고상태·생산진행도
        │   ├── ReleaseController     ← 출고 처리
        │   └── ProductionUtils.h     ← 생산 진척 공유 헬퍼
        ├── view/
        │   ├── MainView      ← 메인 메뉴·서브메뉴·오류 출력
        │   ├── SampleView    ← 시료 입력·목록 출력
        │   ├── OrderView     ← 주문 입력·목록·승인거절 메뉴
        │   ├── ProductionView← 생산 현황 출력 (진행도·완료 예정 시각)
        │   └── MonitorView   ← 모니터링 대시보드 출력
        └── repository/
            ├── ISampleRepository / IOrderRepository  ← 인터페이스
            ├── JsonSampleRepository      ← samples.json CRUD
            ├── JsonOrderRepository       ← orders.json CRUD
            ├── JsonProductionJobRepository← production_jobs.json CRUD
            └── JsonHelper.h              ← JSON 직렬화 헬퍼
```

---

## 아키텍처

**MVC 계층 분리** 원칙을 적용합니다.

```
main.cpp
  └─ Application::run()          ← 메인 루프 (메뉴 선택 → 핸들러 호출)
       ├─ handleSampleMenu()
       ├─ handleOrderReserve()
       ├─ handleOrderApproveReject()
       ├─ handleProduction()
       ├─ handleMonitoring()
       └─ handleRelease()
            │
            ├─ Controller  ─── 비즈니스 로직 (출력 금지)
            ├─ View        ─── 콘솔 입출력 (계산 금지)
            └─ Repository  ─── 파일 I/O (JSON 직렬화)
```

- **Debug 빌드**: `test_runner.cpp` + GoogleTest 100개 TC 실행
- **Release 빌드**: `main.cpp` → `Application` 실행

---

## 주문 상태 전이

```
                ┌─ 재고 충분 ─► CONFIRMED ─► RELEASE
RESERVED ─ 승인 ┤
                └─ 재고 부족 ─► PRODUCING ─► CONFIRMED ─► RELEASE
         ─ 거절 ─► REJECTED
```

| 상태 | 설명 |
|------|------|
| `RESERVED` | 주문 접수 완료, 승인 대기 |
| `CONFIRMED` | 승인 완료, 출고 대기 |
| `PRODUCING` | 생산 중 (재고 부족으로 생산 라인 투입) |
| `RELEASE` | 출고 완료 |
| `REJECTED` | 거절 |

---

## 핵심 계산 공식

### 생산량 계산

```
effectiveStock = stock + getInProgressUnits(queue, nowSec)
availableStock = effectiveStock - reservedQty
shortage       = orderQty - max(0, availableStock)
actualProd     = ceil(shortage / (yield × 0.9))   // 실수율 손실 10% 반영
totalTime(min) = avgProdTime × actualProd
```

### 재고 상태 판단

| 상태 | 조건 |
|------|------|
| `SUFFICIENT` | stock ≥ reservedQty |
| `SHORTAGE` | 0 < stock < reservedQty |
| `DEPLETED` | stock = 0 |

---

## 데이터 영속성

프로그램 재시작 후에도 데이터가 유지됩니다.

| 파일 | 저장 시점 | 복원 내용 |
|------|-----------|-----------|
| `data/samples.json` | 시료 등록·재고 변경 즉시 | 시료 전체 (stock, reservedQty 포함) |
| `data/orders.json` | 주문 상태 변경 즉시 | 주문 전체 (status 포함) |
| `data/production_jobs.json` | 메뉴 루프 종료 시마다 | 생산 큐 (startTime 포함 — 진행률 복원) |

---

## 빌드 및 실행

### 프로그램 실행 (Release)

1. Visual Studio에서 구성을 **Release | x64** 로 설정
2. `Ctrl+Shift+B` 빌드
3. `Ctrl+F5` 실행 (또는 출력 디렉터리에서 `SemiProdiction.exe` 직접 실행)

> **주의**: 실행 파일과 같은 위치에 `data/` 폴더가 있어야 합니다.

### 테스트 실행 (Debug)

1. Visual Studio에서 구성을 **Debug | x64** 로 설정
2. `Ctrl+Shift+B` 빌드
3. `Ctrl+F5` 실행 → GoogleTest 결과 콘솔 출력
4. 또는 **테스트 탐색기**에서 개별 TC 실행 가능

---

## 테스트 현황

| 파일 | TC 수 | 커버 범위 |
|------|-------|-----------|
| test_sample.cpp | 11 | Sample 모델 |
| test_order.cpp | 7 | Order 모델·상태 전이 |
| test_production_job.cpp | 6 | 생산량 공식·진행률 |
| test_sample_controller.cpp | 10 | 시료 등록·검증·검색 |
| test_order_controller.cpp | 14 | 주문 접수·승인·거절 |
| test_production_controller.cpp | 6 | FIFO·tick 완료 |
| test_monitoring_controller.cpp | 9 | 집계·재고상태·생산진행도 |
| test_release_controller.cpp | 5 | 출고·재고 차감 |
| test_json_sample_repository.cpp | 6 | JSON 파일 CRUD |
| test_json_order_repository.cpp | 5 | JSON 파일 CRUD·시퀀스 |
| test_production_stock_flow.cpp | 7 | 생산→완료→출고 전체 흐름 |
| test_stock_aware_approve.cpp | 5 | 생산 진척 반영 실효재고 승인 |
| test_integration.cpp | 4 | 종단간 시나리오 |
| 기타 (smoke, mock) | 5 | 기본 동작 |
| **합계** | **100** | |