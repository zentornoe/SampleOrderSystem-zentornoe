# Phase 3 상세 구현 계획 — 시료 관리 (SampleController + SampleView)

> 참조: [PLAN.md](../PLAN.md) | [PRD.md](../../PRD.md) | [phase_02.md](./phase_02.md)
> 대상 Phase: 3 (SampleController · SampleView)
> 총 테스트: 9개 (Controller: 7개, View: 2개)

---

## 개요

Phase 3은 시료 등록·조회·검색 기능을 제공하는 Controller와 콘솔 출력을 담당하는 View를 구현한다.

**MVC 분리 원칙**
- `SampleController` — 비즈니스 로직 (중복 검사, 유효성 검증, Repository 위임). `std::cout` 사용 금지.
- `SampleView` — 콘솔 출력만 담당. 계산·판단 로직 없음.

**테스트 전략**
- Controller 테스트: `MockSampleRepository` 사용 (파일 I/O 없음)
- View 테스트: `std::cout` 리다이렉션으로 출력 내용 검증

---

## 변경·신규 파일 목록

| 파일 | 작업 | 비고 |
|------|------|------|
| `src/controller/SampleController.h` | `searchByName` 메서드 추가 | 기존 스텁에 누락 |
| `src/controller/SampleController.cpp` | 신규 생성 — 전체 구현 | |
| `src/view/SampleView.cpp` | 신규 생성 — 전체 구현 | |
| `test/test_sample_controller.cpp` | 신규 생성 — 7개 테스트 | |
| `test/test_sample_view.cpp` | 신규 생성 — 2개 테스트 | |
| `SemiProdiction.vcxproj` | 위 .cpp 파일 4개 추가 | |

---

## SampleController 설계

### 헤더 변경 (`SampleController.h`)

기존 스텁에 `searchByName`이 누락되어 있다. 아래를 추가한다.

```
기존:
  registerSample / getSample / getAllSamples / addStock

추가할 선언:
  std::vector<Sample> searchByName(const std::string& keyword) const;
```

`addStock`은 현재 스텁에 존재하지만 Phase 3 테스트 대상이 아니다.
구현은 포함하되 테스트는 Phase 8 (출고 처리) 에서 다룬다.

### 메서드별 구현 규칙

#### `registerSample(id, name, avgProdTime, yield, stock)`

```
1. m_repo.exists(id) == true  →  throw runtime_error("이미 등록된 시료 ID: " + id)
2. Sample s(id, name, avgProdTime, yield, stock)
     └ yield ≤ 0 또는 > 1     →  Sample 생성자가 invalid_argument 발생 (자동 전파)
     └ avgProdTime ≤ 0        →  Sample 생성자가 invalid_argument 발생 (자동 전파)
3. m_repo.save(s)
```

Controller는 중복 ID 검사만 책임진다.
나머지 유효성은 Sample 생성자가 처리하며 컨트롤러는 그대로 전파한다.

#### `getSample(id) const`

```
return m_repo.findById(id)
  └ 없으면 Repository가 runtime_error 발생 (그대로 전파)
```

#### `getAllSamples() const`

```
return m_repo.findAll()
```

#### `searchByName(keyword) const`

```
return m_repo.findByName(keyword)
  └ 결과 없으면 빈 vector 반환 (예외 없음)
```

#### `addStock(id, quantity)` *(Phase 3 구현, 테스트는 Phase 8)*

```
Sample s = m_repo.findById(id)
s.addStock(quantity)       ← quantity ≤ 0 이면 Sample::addStock이 invalid_argument
m_repo.update(s)
```

---

## SampleView 설계

### 메서드별 출력 규칙

현재 스텁에 이미 선언된 메서드만 구현한다.

#### `showSampleList(const std::vector<Sample>& samples) const`

```
samples가 비어있으면:
  "등록된 시료가 없습니다." 출력

비어있지 않으면:
  헤더 행:  "ID | 이름 | 평균생산시간 | 수율 | 재고 | 가용재고"
  각 시료:  id, name, avgProdTime, yield, stock, availableStock 순서로 출력
  최소한 ID, 이름, 재고(stock) 세 항목은 반드시 같은 행에 포함
```

#### `showSample(const Sample& sample) const`

```
단일 시료 상세 정보 출력
ID, 이름, 평균생산시간, 수율, 재고, 가용재고 포함
```

#### `showRegisterSuccess(const std::string& id) const`

```
"[등록 완료] " + id 형식으로 출력
```

#### `showAddStockSuccess(const std::string& id, int quantity) const`

```
"[입고 완료] " + id + " + " + quantity + "개" 형식으로 출력
```

#### 입력 메서드 (`inputId`, `inputName`, `inputYield`, `inputAvgProdTime`, `inputStock`, `inputQuantity`)

```
안내 문자열 출력 후 std::cin >> 으로 값 입력받아 반환
```

---

## Controller 테스트 설계 (7개)

**파일**: `test/test_sample_controller.cpp`

**공통 준비 코드** (각 테스트에서 반복):

```cpp
MockSampleRepository mockRepo;
SampleController ctrl(mockRepo);
```

---

### 테스트 1 — 정상 등록 시 Repository의 save가 1회 호출되는지

```
GIVEN: exists("S-001") → false
WHEN:  ctrl.registerSample("S-001", "실리콘 웨이퍼", 0.5, 0.92, 480)
THEN:  mockRepo.save() 가 정확히 1회 호출됨
```

GMock 설정:
```cpp
EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(false));
EXPECT_CALL(mockRepo, save(testing::_)).Times(1);
```

---

### 테스트 2 — 중복 ID 등록 시 runtime_error 발생

```
GIVEN: exists("S-001") → true
WHEN:  ctrl.registerSample("S-001", ...)
THEN:  std::runtime_error 발생
       mockRepo.save() 는 호출되지 않음
```

GMock 설정:
```cpp
EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(true));
EXPECT_CALL(mockRepo, save(testing::_)).Times(0);
```

---

### 테스트 3 — 수율이 범위(0 초과 ~ 1 이하)를 벗어나면 invalid_argument 발생

```
GIVEN: exists("S-001") → false, yield = 1.5
WHEN:  ctrl.registerSample("S-001", "test", 0.5, 1.5, 100)
THEN:  std::invalid_argument 발생 (Sample 생성자에서 전파)
```

GMock 설정:
```cpp
EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(false));
// save는 호출되지 않음
```

경계값 추가 검증: `yield = 0.0` 도 동일한 예외 발생 확인.

---

### 테스트 4 — 평균 생산시간이 0 이하이면 invalid_argument 발생

```
GIVEN: exists("S-001") → false, avgProdTime = -1.0
WHEN:  ctrl.registerSample("S-001", "test", -1.0, 0.9, 100)
THEN:  std::invalid_argument 발생 (Sample 생성자에서 전파)
```

GMock 설정:
```cpp
EXPECT_CALL(mockRepo, exists("S-001")).WillOnce(Return(false));
```

---

### 테스트 5 — getAllSamples 가 Repository 결과를 그대로 반환하는지

```
GIVEN: findAll() → [Sample("S-001",...), Sample("S-002",...)]
WHEN:  ctrl.getAllSamples()
THEN:  반환된 vector의 size == 2
       result[0].getId() == "S-001"
       result[1].getId() == "S-002"
```

GMock 설정:
```cpp
std::vector<Sample> samples = {
    Sample("S-001", "웨이퍼A", 0.5, 0.9, 100),
    Sample("S-002", "웨이퍼B", 1.0, 0.8, 200)
};
EXPECT_CALL(mockRepo, findAll()).WillOnce(Return(samples));
```

---

### 테스트 6 — searchByName 이 Repository에 키워드를 그대로 위임하는지

```
GIVEN: findByName("웨이퍼") → [Sample("S-001",...), Sample("S-002",...)]
WHEN:  ctrl.searchByName("웨이퍼")
THEN:  반환된 vector의 size == 2
```

GMock 설정:
```cpp
std::vector<Sample> matched = { sample1, sample2 };
EXPECT_CALL(mockRepo, findByName("웨이퍼")).WillOnce(Return(matched));
```

---

### 테스트 7 — 검색 결과가 없을 때 빈 vector를 반환하는지

```
GIVEN: findByName("없는키워드") → []
WHEN:  ctrl.searchByName("없는키워드")
THEN:  result.empty() == true  (예외 발생하지 않음)
```

GMock 설정:
```cpp
EXPECT_CALL(mockRepo, findByName(testing::_))
    .WillOnce(Return(std::vector<Sample>{}));
```

---

## View 테스트 설계 (2개)

**파일**: `test/test_sample_view.cpp`

`std::cout` 출력을 `std::ostringstream`으로 가로채는 패턴:

```cpp
std::ostringstream oss;
std::streambuf* oldBuf = std::cout.rdbuf(oss.rdbuf());  // 리다이렉션
sampleView.showSampleList(samples);
std::cout.rdbuf(oldBuf);                                 // 복원
std::string output = oss.str();
```

---

### 테스트 8 — showSampleList: 출력에 ID·이름·재고가 포함되는지

```
GIVEN: [Sample("S-001", "실리콘 웨이퍼", 0.5, 0.92, 480)]
WHEN:  view.showSampleList(samples)  (stdout 리다이렉션)
THEN:  출력 문자열에 "S-001" 포함
       출력 문자열에 "실리콘 웨이퍼" 포함
       출력 문자열에 "480" 포함
```

---

### 테스트 9 — showSampleList: 빈 목록일 때 안내 문구가 출력되는지

```
GIVEN: 빈 vector
WHEN:  view.showSampleList({})  (stdout 리다이렉션)
THEN:  출력 문자열에 "없습니다" (또는 "empty" 등 안내 문구) 포함
```

---

## vcxproj 추가 항목

### 항상 컴파일 (Debug + Release 모두)

```xml
<ClCompile Include="src\controller\SampleController.cpp" />
<ClCompile Include="src\view\SampleView.cpp" />
```

### Debug 전용 테스트

```xml
<ClCompile Include="test\test_sample_controller.cpp"
           Condition="'$(Configuration)'=='Debug'" />
<ClCompile Include="test\test_sample_view.cpp"
           Condition="'$(Configuration)'=='Debug'" />
```

---

## 완료 기준

| 조건 | 기준 |
|------|------|
| 빌드 | Debug / Release 모두 오류 0 |
| 테스트 | 누적 41개 통과 (Phase 0~2의 32개 + Phase 3의 9개) |
| MVC 분리 | SampleController에 `std::cout` 없음, SampleView에 비즈니스 로직 없음 |
| Mock 활용 | Controller 테스트 전부 MockSampleRepository 사용, 파일 I/O 없음 |

---

## 검토 포인트

| 항목 | 설명 |
|------|------|
| `searchByName` 스텁 누락 | `SampleController.h`에 선언 추가 필수. 누락 시 컴파일 에러 |
| 유효성 검증 위치 | yield·avgProdTime은 Sample 생성자에서 검증. 컨트롤러는 중복 ID 검사만 추가 |
| `getSample` 예외 전파 | `findById` 없는 ID는 Repository가 `runtime_error` 발생. 컨트롤러는 catch 없이 전파 |
| View 테스트 격리 | `std::cout.rdbuf` 교체 후 반드시 복원. 복원 누락 시 이후 테스트 출력 오염 |
| `addStock` 구현 범위 | Phase 3에서 구현하되 테스트는 Phase 8에서 진행. 현재 빈 구현(stub) 상태여도 무방 |
| 안내 문구 정확한 문자열 | 테스트 9에서 `"없습니다"` 등 일부 문자열만 포함 여부를 검증 (`EXPECT_NE(output.find("없습니다"), npos)`) |
