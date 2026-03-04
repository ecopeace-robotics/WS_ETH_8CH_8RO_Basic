# Doxygen 사용 가이드
> ESP32-S3 Ethernet Motor Controller 프로젝트를 예제로 배우는 Doxygen 실전 입문

---

## 목차

1. [Doxygen이란?](#1-doxygen이란)
2. [Doxygen의 장점](#2-doxygen의-장점)
3. [설치](#3-설치)
4. [Doxyfile 생성 및 핵심 설정](#4-doxyfile-생성-및-핵심-설정)
5. [주석 작성법](#5-주석-작성법)
6. [문서 생성 및 확인](#6-문서-생성-및-확인)
7. [재사용 절차 요약 (체크리스트)](#7-재사용-절차-요약-체크리스트)

---

## 1. Doxygen이란?

**Doxygen**은 C, C++, Python 등 다양한 언어의 소스코드에서 주석을 읽어
HTML, PDF 등의 형태로 **API 레퍼런스 문서를 자동 생성**해 주는 도구이다.

소스코드에 특정 형식(`/** ... */`)으로 주석을 달면, Doxygen이 이를 파싱해
함수 목록, 파라미터 설명, 콜 그래프(call graph), 파일 구조 등을 시각화한다.

```
소스코드 (.c / .h)
  └─ Doxygen 주석 (/** @brief ... */)
        └─ doxygen Doxyfile 실행
              └─ html/index.html  (브라우저에서 열람)
```

---

## 2. Doxygen의 장점

### 2.1 문서 유지보수 비용 절감
- 주석이 곧 문서이므로 코드와 문서가 **항상 같은 파일에 존재**한다.
- 코드를 수정할 때 주석도 함께 수정하는 습관이 생기므로 문서 이탈(drift)이 줄어든다.
- README에 모든 API를 수동으로 복사할 필요가 없다.

### 2.2 코드 가독성 향상
- 함수마다 `@brief`, `@param`, `@return`을 강제하면 **함수 역할이 명확**해진다.
- 나중에 코드를 다시 볼 때, 또는 팀원이 처음 볼 때 학습 비용이 낮아진다.

### 2.3 시각적 콜 그래프 (Call Graph)
- Graphviz 연동 시 **어떤 함수가 어디서 호출되는지** 다이어그램으로 볼 수 있다.
- 예: `app_main()` → `exio_init()` → `exio_write_port()` 흐름이 그래프로 표시된다.
- 리팩토링이나 버그 추적 시 함수 간 의존성을 파악하기 쉽다.

### 2.4 상수·열거형 그룹화 (`@defgroup`)
- `board_config.h`처럼 `#define`이 많은 파일에서 **논리적 그룹으로 묶어** 표시할 수 있다.
- 예: Ethernet GPIO 핀, 네트워크 설정, PWM 파라미터를 각각 별도 섹션으로 분리.

### 2.5 재현 가능한 산출물
- `Doxyfile`과 주석이 달린 소스만 Git에 커밋하면, 누구든지 `doxygen Doxyfile` 한 줄로 동일한 문서를 재생성할 수 있다.
- CI/CD 파이프라인에 추가하면 매 빌드마다 최신 문서가 자동 배포된다.

### 2.6 ESP-IDF 프로젝트와의 궁합
- ESP-IDF 자체도 Doxygen 주석 체계를 사용하므로 **스타일이 일관**된다.
- `esp_err_t` 반환값 패턴, `ESP_OK` / `ESP_ERR_*` 설명을 표준화하기 좋다.

---

## 3. 설치

Ubuntu/Debian 기준 (1회만 설치):

```bash
sudo apt install doxygen graphviz

# 설치 확인
doxygen --version   # 예: 1.9.1
dot -V              # 예: graphviz version 2.43.0
```

- `doxygen` — 문서 생성 엔진
- `graphviz` — 콜 그래프·의존성 다이어그램 렌더링 (`dot` 명령)

---

## 4. Doxyfile 생성 및 핵심 설정

### 4.1 기본 Doxyfile 생성

프로젝트 루트에서 실행하면 약 2,500줄짜리 기본 설정 파일이 생성된다:

```bash
cd /path/to/project
doxygen -g Doxyfile
```

### 4.2 반드시 변경해야 할 설정 항목

아래 항목들을 텍스트 에디터로 열어 수정한다.

#### 프로젝트 정보

```ini
PROJECT_NAME    = "ESP32-S3 Ethernet Motor Controller"
PROJECT_NUMBER  = 1.0.0
PROJECT_BRIEF   = "TCP-controlled 2-ch PWM motor + 8-ch relay via W5500 Ethernet"
```

#### 입력 소스 경로

```ini
INPUT      = main        # 소스가 있는 디렉터리 (프로젝트마다 변경)
RECURSIVE  = YES         # 하위 폴더까지 탐색
EXCLUDE    = build       # 빌드 산출물 제외
```

> **이 프로젝트에서의 실수 경험:**
> `INPUT`을 비워두면 Doxygen이 프로젝트 루트를 스캔하므로 `conversations.md`,
> `README.md` 같은 파일만 처리되고 정작 `main/` 안의 C 소스는 빠진다.
> 반드시 `INPUT = main` 처럼 소스 디렉터리를 명시하라.

#### 인코딩 (한글 포함 소스 필수)

```ini
DOXYFILE_ENCODING = UTF-8
INPUT_ENCODING    = UTF-8
```

#### 추출 범위

```ini
EXTRACT_ALL     = YES    # 주석 없는 함수도 문서에 포함
EXTRACT_STATIC  = YES    # static 함수도 포함
EXTRACT_PRIVATE = YES    # private 멤버도 포함
```

#### 콜 그래프 (Graphviz 필요)

```ini
HAVE_DOT            = YES
CALL_GRAPH          = YES    # 이 함수가 호출하는 함수 그래프
CALLER_GRAPH        = YES    # 이 함수를 호출하는 함수 그래프
DOT_GRAPH_MAX_NODES = 50
```

#### HTML 출력

```ini
GENERATE_HTML    = YES
GENERATE_TREEVIEW = YES    # 좌측 탐색 패널 활성화
HTML_TIMESTAMP   = YES
```

#### LaTeX/PDF 비활성화 (불필요 시)

```ini
GENERATE_LATEX = NO
```

#### 매크로 전처리 (ESP-IDF 빌드 매크로 처리)

```ini
ENABLE_PREPROCESSING = YES
MACRO_EXPANSION      = YES
```

---

## 5. 주석 작성법

### 5.1 파일 헤더 (`@file`)

모든 `.h` / `.c` 파일의 **첫 번째 줄**에 추가한다.
이것이 없으면 Doxygen이 해당 파일을 문서 인덱스에 올바르게 표시하지 못한다.

```c
/**
 * @file pwm_ctrl.h
 * @brief LEDC 기반 RC PWM 드라이버 (2채널, 50 Hz, 1000–2000 µs)
 *        Cytron MDDS30 같은 RC ESC / 서보 인터페이스와 호환된다.
 */
```

**이 프로젝트 예시:**

| 파일 | @brief 내용 |
|---|---|
| `board_config.h` | 하드웨어 핀 배정 및 컴파일타임 상수 |
| `eth_init.h` | W5500 SPI Ethernet 초기화 |
| `exio.h` | TCA9554PWR I2C IO 익스팬더 드라이버 |
| `relay_ctrl.h` | 릴레이 고수준 제어 API |
| `pwm_ctrl.h` | LEDC 기반 RC PWM 출력 드라이버 |
| `tcp_server.h` | TCP 서버 태스크 |
| `main.c` | 애플리케이션 진입점, 초기화 순서 |

---

### 5.2 함수 주석 (`@brief`, `@param`, `@return`)

```c
/**
 * @brief 지정 채널의 펄스 폭 설정
 * @param channel  채널 번호, 1 또는 2
 * @param pulse_us 펄스 폭 (µs), 범위 1000~2000 (초과 시 자동 클램프)
 * @return ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t pwm_set_us(uint8_t channel, uint16_t pulse_us);
```

**규칙 요약:**

| 태그 | 설명 | 필수 여부 |
|---|---|---|
| `@brief` | 한 줄 요약 (첫 문장이 자동으로 사용됨) | 필수 |
| `@param` | 각 파라미터 설명. 이름과 설명을 공백으로 구분 | 파라미터 있을 때 |
| `@return` | 반환값 의미. `void`면 생략 | 반환값 있을 때 |
| `@note` | 사용 시 주의사항 | 선택 |
| `@see` | 관련 함수 참조 | 선택 |

---

### 5.3 상수 그룹화 (`@defgroup`)

`board_config.h`처럼 `#define`이 많을 때, 논리적 묶음으로 분리하면
문서에서 **Modules** 탭에 그룹별로 깔끔하게 표시된다.

```c
/** @defgroup CFG_PWM RC PWM 출력 설정 (LEDC)
 *  50 Hz, 14-bit 해상도, 펄스 범위 1000–2000 µs, 중립 1500 µs.
 *  @{
 */
#define PWM_CH1_GPIO    47      /**< PWM 채널 1 출력 GPIO */
#define PWM_FREQ_HZ     50      /**< PWM 주파수 (Hz) */
#define PWM_US_NEUTRAL  1500    /**< 중립 펄스 폭 (µs) */
/** @} */
```

**이 프로젝트의 그룹 구성:**

| 그룹 이름 | 내용 |
|---|---|
| `CFG_ETH` | W5500 SPI 핀 및 클럭 설정 |
| `CFG_EXIO` | TCA9554PWR I2C IO 익스팬더 |
| `CFG_TCA9554_REGS` | TCA9554 내부 레지스터 주소 |
| `CFG_NET` | 정적 IP / DHCP 네트워크 설정 |
| `CFG_PWM` | RC PWM 출력 파라미터 |
| `CFG_TCP` | TCP 서버 포트 및 버퍼 크기 |

---

### 5.4 열거형 주석

`typedef enum`에는 전체 설명 블록 + 각 멤버에 `/**< ... */` 인라인 주석을 붙인다.

```c
/**
 * @brief 릴레이 채널 식별자.
 *        RELAY_1은 TCA9554 bit 0(EXIO1), RELAY_8은 bit 7(EXIO8)에 대응된다.
 */
typedef enum {
    RELAY_1 = 0,   /**< EXIO1 — bit 0 */
    RELAY_2,       /**< EXIO2 — bit 1 */
    /* ... */
    RELAY_MAX      /**< Sentinel — 채널 ID로 사용 금지 */
} relay_id_t;
```

---

### 5.5 `.c` 구현 파일

구현 파일에는 최소한 `@file` 블록만 추가하면 된다.
API 주석은 헤더(`.h`)에, 알고리즘 설명은 `.c` 내부 인라인 주석으로 작성한다.

```c
/**
 * @file main.c
 * @brief 애플리케이션 진입점 — 모든 서브시스템을 순서대로 초기화하고
 *        FreeRTOS 스케줄러에 제어권을 넘긴다.
 *
 * 초기화 순서:
 *  1. NVS 플래시 (Ethernet MAC 주소 저장)
 *  2. 기본 이벤트 루프
 *  3. esp_netif
 *  4. EXIO / TCA9554 IO 익스팬더
 *  5. 릴레이 컨트롤러 (전체 OFF)
 *  6. RC PWM 출력 (중립 1500 µs)
 *  7. W5500 Ethernet (IP 획득 시 TCP 서버 자동 시작)
 */
```

---

## 6. 문서 생성 및 확인

### 6.1 생성 실행

```bash
cd /path/to/project
doxygen Doxyfile
```

정상 완료 시 마지막 줄에 `finished...`가 출력된다.
오류(error)가 없고 경고(warning)만 있으면 문서는 생성된다.

### 6.2 브라우저로 열기

```bash
xdg-open html/index.html    # Linux
open html/index.html         # macOS
```

### 6.3 생성된 주요 페이지

| HTML 파일 | 내용 |
|---|---|
| `index.html` | 메인 페이지 |
| `files.html` | 소스 파일 인덱스 |
| `modules.html` | `@defgroup`으로 정의한 모듈 목록 |
| `globals_func.html` | 전역 함수 목록 |
| `globals_defs.html` | `#define` 매크로 목록 |
| `group__CFG__PWM.html` | CFG_PWM 그룹 상세 |
| `pwm__ctrl_8h.html` | `pwm_ctrl.h` 상세 문서 |

### 6.4 .gitignore 등록

생성된 HTML은 `doxygen Doxyfile`로 재현 가능하므로 Git에 포함하지 않는다:

```
# .gitignore에 추가
html/
latex/
```

Git에는 `Doxyfile`과 주석이 추가된 소스 파일만 커밋한다.

---

## 7. 재사용 절차 요약 (체크리스트)

새 ESP-IDF 프로젝트에 Doxygen을 적용할 때 아래 순서를 따른다.

```
[ ] 1. 설치 (최초 1회)
        sudo apt install doxygen graphviz

[ ] 2. 기본 Doxyfile 생성
        doxygen -g Doxyfile

[ ] 3. Doxyfile 핵심 항목 수정
        PROJECT_NAME    = "프로젝트명"
        PROJECT_NUMBER  = x.y.z
        PROJECT_BRIEF   = "한 줄 설명"
        INPUT           = main          ← 소스 디렉터리 반드시 지정
        RECURSIVE       = YES
        EXCLUDE         = build
        DOXYFILE_ENCODING = UTF-8
        INPUT_ENCODING  = UTF-8
        EXTRACT_ALL     = YES
        EXTRACT_STATIC  = YES
        HAVE_DOT        = YES
        CALL_GRAPH      = YES
        CALLER_GRAPH    = YES
        GENERATE_LATEX  = NO
        GENERATE_TREEVIEW = YES

[ ] 4. 모든 .h / .c 파일 상단에 @file 블록 추가

[ ] 5. 모든 공개 함수에 @brief / @param / @return 추가

[ ] 6. #define이 많은 헤더에 @defgroup / @{ @} 추가

[ ] 7. typedef enum에 전체 블록 + /**< */ 인라인 주석 추가

[ ] 8. 문서 생성 및 확인
        doxygen Doxyfile
        xdg-open html/index.html

[ ] 9. .gitignore에 html/, latex/ 추가

[ ] 10. Doxyfile + 소스 파일만 Git 커밋
```

---

## 부록 — 자주 하는 실수

| 실수 | 증상 | 해결 |
|---|---|---|
| `INPUT` 미설정 | C 파일 문서가 하나도 없고 .md 파일만 나옴 | `INPUT = main` 명시 |
| `RECURSIVE = NO` | 하위 폴더 소스가 빠짐 | `RECURSIVE = YES` |
| `@file` 누락 | 파일이 문서 인덱스에 표시되지 않음 | 모든 .h/.c 상단에 `@file` 추가 |
| 한글 깨짐 | 주석이 깨진 문자로 표시됨 | `INPUT_ENCODING = UTF-8` 설정 |
| `HAVE_DOT = NO` | 콜 그래프가 표시되지 않음 | `HAVE_DOT = YES` + graphviz 설치 |
| 생성 디렉터리 미확인 | `html/` 위치를 모름 | `grep OUTPUT_DIRECTORY Doxyfile`로 확인 |
