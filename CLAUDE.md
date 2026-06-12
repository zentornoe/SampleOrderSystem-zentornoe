# CLAUDE.md — 반도체 시료 생산주문관리 시스템

> 이 파일은 Claude Code가 프로젝트를 이해하고 작업할 때 참조하는 가이드입니다.
> 기능 명세 전체는 [PRD.md](./PRD.md) 를 참조하세요.

---

## 프로젝트 한 줄 요약

**S-Semi** 반도체 회사의 시료(Sample) 주문·생산·출고 흐름을 관리하는 **C++ 콘솔 애플리케이션**.

---

## 개발 환경

| 항목 | 내용 |
|------|------|
| IDE | Visual Studio 2022 (또는 2019) |
| 언어 | C++ 17 이상 |
| 빌드 | MSBuild (Visual Studio Solution / .vcxproj) |
| 실행 환경 | Windows 콘솔 (cmd / PowerShell) |
| 데이터 저장 | JSON 파일 (재실행 후에도 데이터 유지) |

---

## 프로젝트 구조

```
SemiProdiction/
├── CLAUDE.md              # Claude Code 가이드 (이 파일)
├── PRD.md                 # 기능 명세 및 요구사항 전체 문서
├── SemiProdiction.sln     # Visual Studio 솔루션 파일
├── src/
│   ├── main.cpp           # 진입점, 메인 루프
│   ├── model/             # 도메인 객체
│   │   ├── Sample.h / .cpp        # 시료 (ID, 이름, 수율, 생산시간, 재고)
│   │   ├── Order.h / .cpp         # 주문 (주문번호, 상태, 고객, 수량)
│   │   └── ProductionLine.h / .cpp # 생산라인 (FIFO 큐, 생산 현황)
│   ├── controller/        # 비즈니스 로직
│   │   ├── SampleController.h / .cpp   # 시료 등록·조회·검색
│   │   ├── OrderController.h / .cpp    # 주문 접수·승인·거절
│   │   ├── ProductionController.h / .cpp # 생산라인 처리
│   │   └── ReleaseController.h / .cpp  # 출고 처리
│   ├── view/              # 콘솔 화면 출력
│   │   ├── MainView.h / .cpp      # 메인 메뉴 화면
│   │   ├── SampleView.h / .cpp    # 시료 관리 화면
│   │   ├── OrderView.h / .cpp     # 주문 화면
│   │   ├── MonitorView.h / .cpp   # 모니터링 화면
│   │   └── ProductionView.h / .cpp # 생산라인 화면
│   └── repository/        # 데이터 영속성
│       ├── SampleRepository.h / .cpp  # 시료 데이터 파일 I/O
│       └── OrderRepository.h / .cpp   # 주문 데이터 파일 I/O
└── data/
    ├── samples.json        # 시료 데이터 저장소
    └── orders.json         # 주문 데이터 저장소
```

---

## 핵심 비즈니스 규칙 (코드 작성 시 반드시 준수)

### 주문 상태 전이
```
RESERVED → 승인 → 재고 충분  → CONFIRMED → RELEASE
                → 재고 부족  → PRODUCING → CONFIRMED → RELEASE
         → 거절 → REJECTED
```

### 생산량 계산 공식
```cpp
int shortage     = orderQty - currentStock;           // 부족분
int actualProd   = (int)ceil(shortage / (yield * 0.9)); // 실 생산량
double totalTime = avgProdTime * actualProd;            // 총 생산 시간(min)
```

### 재고 상태 판단
- **여유**: 현재 재고 ≥ 주문 수량
- **부족**: 0 < 현재 재고 < 주문 수량
- **고갈**: 현재 재고 = 0

### 생산라인 스케줄링
- FIFO (`std::queue` 사용)
- 단일 라인: 한 번에 하나의 주문만 처리
- 생산 완료 시 `PRODUCING` → `CONFIRMED` 자동 전환

---

## 코딩 컨벤션

- 클래스명: PascalCase (`SampleController`)
- 멤버 변수: camelCase with `m_` prefix (`m_sampleId`)
- 함수명: camelCase (`registerSample`)
- 상수 / enum: UPPER_SNAKE_CASE (`ORDER_STATUS::RESERVED`)
- 주석: 비즈니스 로직의 "왜(Why)"만 작성, "무엇(What)"은 생략

---

## 작업 원칙

1. **기능 명세 우선** — PRD.md 명세를 벗어나는 기능 임의 추가 금지
2. **MVC 분리 유지** — View에 비즈니스 로직 작성 금지, Controller에 출력 코드 작성 금지
3. **데이터 영속성** — 모든 변경은 즉시 파일에 반영 (재실행 시 데이터 복원 보장)
4. **에러 처리** — 잘못된 입력(없는 시료 ID, 음수 수량 등)은 메시지 출력 후 메뉴로 복귀

---

## 주요 참조 문서

- **[PRD.md](./PRD.md)** — 전체 기능 명세, 상태 흐름, UI 예시, 미션 구성 및 제출 방법
