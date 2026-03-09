# Doxygen 주석 작성 표준

> ESP-IDF / ESP32 개발팀 내부 문서화 가이드라인

**핵심 원칙:** 주석만 잘 달면, 나머지는 자동으로 처리된다.
Doxygen이 함수 관계도, 파라미터 표, 흐름도를 자동 생성한다.

---

## 목차

1. [적용 범위](#1-적용-범위)
2. [기본 문법](#2-기본-문법)
3. [파일 헤더 템플릿](#3-파일-헤더-템플릿)
4. [함수 주석 템플릿](#4-함수-주석-템플릿)
5. [구조체 / 열거형 / 매크로](#5-구조체--열거형--매크로)
6. [태그 전체 참조](#6-태그-전체-참조)
7. [문서 구조 태그 — @mainpage / @page](#7-문서-구조-태그--mainpage--page)
8. [모듈 그룹화 — @defgroup / @ingroup](#8-모듈-그룹화--defgroup--ingroup)
9. [파일별 작성 우선순위 요약](#9-파일별-작성-우선순위-요약)
10. [커밋 전 체크리스트](#10-커밋-전-체크리스트)
11. [태그 빠른 참조 카드](#11-태그-빠른-참조-카드)

---

## 1. 적용 범위

| 대상 | 필수 여부 | 설명 |
|---|---|---|
| `.h` 파일 공용 함수 | **필수** | 외부에서 호출되는 모든 API |
| `.h` 파일 구조체/열거형 | **필수** | 모든 public 타입 정의 |
| 파일 헤더 (`.h` / `.c`) | **필수** | 모든 소스 파일 |
| `app_main()` | **필수** | 진입점 및 태스크 구조 기술 |
| `.c` 파일 `static` 함수 | 권장 | 간단한 `@brief` 한 줄 이상 |

---

## 2. 기본 문법

모든 Doxygen 주석은 `/**`로 시작하고 `*/`로 닫는다.
구조체/열거형 멤버의 인라인 주석은 `/**<`를 사용한다.

```c
/* 일반 C 주석 — Doxygen이 무시함 */

/** Doxygen이 파싱하는 블록 주석
 * 여러 줄 작성 가능
 */

typedef struct {
    int channel;  /**< ADC 채널 번호 (멤버 인라인 주석) */
    int samples;  /**< 평균낼 샘플 수 */
} sensor_config_t;
```

---

## 3. 파일 헤더 템플릿

### .h 파일 — API 목적과 사용법 중심

```c
/**
 * @file    sensor_adc.h
 * @brief   ADC 기반 센서 읽기 모듈 공개 API
 *
 * @details
 * ADC1 채널을 초기화하고 멀티샘플링 평균값을 반환한다.
 * ESP32-S3 기준 12bit 해상도 사용.
 * Wi-Fi와 충돌하는 ADC2는 사용하지 않는다.
 *
 * 사용 예시:
 * @code
 * sensor_adc_init(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
 * int mv = 0;
 * sensor_adc_read(&mv);
 * @endcode
 *
 * @author  홍길동
 * @date    2025-03-05
 */
```

### .c 파일 — 구현 세부사항 중심

```c
/**
 * @file    sensor_adc.c
 * @brief   sensor_adc.h 구현부
 *
 * @details
 * Espressif ADC Oneshot Driver API 사용.
 * 내부적으로 10회 샘플링 후 평균값 산출.
 * 이상값(±20% 초과) 자동 필터링 포함.
 *
 * @author  홍길동
 * @date    2025-03-05
 */
```

### main.c / app_main() — 진입점 특수 처리

> **주의:** `app_main`은 `@param`, `@return` 태그가 없다.
> 파일 헤더는 초기화 순서를 텍스트로 간결하게, 함수 주석에 상세 흐름도를 담는다.

```c
/**
 * @file    main.c
 * @brief   애플리케이션 진입점 — 모든 서브시스템을 순서대로 초기화하고
 *          FreeRTOS 스케줄러에 제어를 넘긴다.
 *
 * @details
 * 초기화 순서:
 *  1. NVS 플래시 (설정값 저장)
 *  2. 기본 이벤트 루프
 *  3. esp_netif 초기화
 *  4. (서브시스템 초기화 — I2C, SPI, GPIO, 드라이버 등)
 *  5. Ethernet 초기화 (IP 획득 후 응용 태스크 시작)
 */

/**
 * @brief   FreeRTOS 태스크 및 드라이버를 초기화하고 스케줄러에 제어를 넘긴다.
 *
 * @details
 * 부팅 흐름:
 * \dot
 * digraph BootFlow {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     Bootloader      [style=filled, fillcolor=lightgray];
 *     Scheduler       [label="FreeRTOS Scheduler", style=filled, fillcolor=lightgray];
 *     AppTask         [label="응용 태스크", style=filled, fillcolor=lightyellow];
 *
 *     Bootloader -> nvs_flash_init -> esp_event_loop -> esp_netif_init;
 *     esp_netif_init -> exio_init -> relay_init -> pwm_init -> eth_init;
 *     eth_init -> Scheduler [label="반환"];
 *     eth_init -> AppTask [label="IP 획득 (비동기, 최초 1회)", style=dashed];
 * }
 * \enddot
 *
 * @note 이 함수는 반환되지 않는다.
 *       모든 비즈니스 로직은 별도 FreeRTOS 태스크로 분리되어 있다.
 *       응용 태스크는 Ethernet IP 획득 이후 자동으로 시작된다.
 */
void app_main(void);
```

#### 파일 헤더 vs 함수 주석: 역할 구분

**왜 파일 헤더와 함수 주석을 나누는가?**

- **파일 헤더(`@file`)**: "이 파일의 목적은 무엇인가?" — IDE에서 파일을 열기 **전에** 검색 결과나 파일 목록에서 읽는 정보다. 간결한 텍스트 수준의 개요로 충분하다.

- **함수 주석**: "이 함수가 어떻게 동작하는가?" — 함수를 호출하거나 수정할 때 읽는 정보다. 복잡한 초기화 흐름이나 상태 전이는 Graphviz 다이어그램으로 명확하게 표현한다.

**잘못된 예 vs 올바른 예:**

```c
// ❌ 파일 헤더에 복잡한 Graphviz 다이어그램
/**
 * @file main.c
 * @details
 * \dot
 * digraph VeryComplex { ... (50줄) ... }
 * \enddot
 */
// 파일 목록에서 이 파일을 선택하기 전부터 복잡한 다이어그램을 읽어야 함

// ✅ 파일 헤더는 간결한 텍스트, 함수 주석에 흐름도
/**
 * @file main.c
 * @brief 애플리케이션 진입점
 * @details
 * 초기화 순서:
 *  1. NVS 플래시
 *  2. 이벤트 루프
 *  3. ...
 */

/**
 * @brief FreeRTOS를 초기화하고 스케줄러에 제어를 넘긴다.
 * @details
 * \dot
 * digraph BootFlow { ... }
 * \enddot
 */
void app_main(void);
// 함수 코드를 볼 때만 상세한 흐름도가 필요하므로 여기에 배치
```

---

## 4. 함수 주석 템플릿

### .h 파일 — 공용 API 전체 명세

```c
/**
 * @brief   ADC 채널을 초기화한다.
 *
 * @param[in]   channel   초기화할 ADC1 채널 번호
 * @param[in]   atten     감쇠 설정 (adc_atten_t)
 *
 * @return
 *   - ESP_OK   : 초기화 성공
 *   - ESP_FAIL : 채널 설정 실패
 *
 * @note ADC2는 Wi-Fi 사용 중 충돌 가능. ADC1만 사용할 것.
 *
 * @see sensor_adc_read()
 */
esp_err_t sensor_adc_init(adc1_channel_t channel, adc_atten_t atten);
```

### .c 파일 — 내부 구현 흐름 (복잡한 함수만)

`.h`와 `.c` 모두에 동일한 `@param` / `@return`을 쓰지 않는다.
API 명세는 `.h`에만 작성하고, `.c`에는 구현 설명만 추가한다.

```c
/**
 * @brief   센서값을 읽어 필터링 후 반환한다.
 *
 * @details
 * 처리 흐름:
 * \dot
 * digraph ReadFlow {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     A [label="ADC Raw Read x10"];
 *     B [label="평균값 계산"];
 *     C [label="범위 초과?", shape=diamond];
 *     D [label="ESP_ERR_INVALID_RESPONSE"];
 *     E [label="calibrated 값 반환"];
 *     A -> B;
 *     B -> C;
 *     C -> D [label="Yes"];
 *     C -> E [label="No"];
 * }
 * \enddot
 *
 * @note 파라미터/반환값 명세는 .h 주석 참조.
 */
esp_err_t sensor_adc_read(int *out_value) { ... }
```

---

## 5. 구조체 / 열거형 / 매크로

### 구조체

```c
/**
 * @brief   ADC 센서 설정 구조체
 * @ingroup SensorADC
 */
typedef struct {
    adc1_channel_t  channel;  /**< ADC1 채널 번호 */
    adc_atten_t     atten;    /**< 입력 감쇠 설정 */
    uint8_t         samples;  /**< 평균낼 샘플 수 (1~64) */
} sensor_adc_config_t;
```

### 열거형

```c
/**
 * @brief   센서 동작 상태
 */
typedef enum {
    SENSOR_STATE_IDLE    = 0,  /**< 초기화 전 또는 대기 상태 */
    SENSOR_STATE_RUNNING = 1,  /**< 정상 측정 중 */
    SENSOR_STATE_ERROR   = 2,  /**< 오류 발생 */
} sensor_state_t;
```

### 매크로

```c
/** ADC 최대 샘플 수. 이 값을 초과하면 assert 발생. */
#define SENSOR_ADC_MAX_SAMPLES  64
```

---

## 6. 태그 전체 참조

### 문서 구조 태그

| 태그 | 용도 | 필수 여부 | 비고 |
|---|---|---|---|
| `@mainpage` | Doxygen HTML 표지 생성 | **필수** (1개) | `.dox` 또는 `README.md`에 작성 |
| `@page` | 주제별 독립 페이지 생성 | 권장 | Related Pages 섹션에 표시 |
| `@subpage` | 페이지 간 계층 연결/링크 | 권장 | `@mainpage` 또는 `@page` 안에서 사용 |
| `@section` | 페이지 내 소제목 구분 | 선택 | `@page` / `@mainpage` 안에서 사용 |
| `@ref` | 다른 심볼/페이지로 링크 | 선택 | 그룹, 함수, 페이지 모두 링크 가능 |

### 기본 설명 태그

| 태그 | 용도 | 필수 여부 | 비고 |
|---|---|---|---|
| `@brief` | 한 줄 요약 | **필수** | Doxygen 목록에서 표시됨 |
| `@details` | 상세 설명, 여러 줄 가능 | 선택 | `@brief` 이후 작성 |
| `@note` | 주의사항, 제약 조건 | 선택 | 별도 박스로 표시됨 |
| `@warning` | 잘못 쓰면 시스템 오류 수준 경고 | 선택 | 붉은 경고 박스 |
| `@attention` | 반드시 읽어야 할 중요 주의 | 선택 | `@warning`보다 강조 |
| `@remark` | 추가적인 설명이나 관찰 사항 | 선택 | 일반 텍스트로 표시 |

### 파라미터 및 반환값 태그

| 태그 | 용도 | 필수 여부 | 비고 |
|---|---|---|---|
| `@param[in]` | 입력 파라미터 (함수가 읽기만) | **필수** | 입력 파라미터에 사용 |
| `@param[out]` | 출력 파라미터 (함수가 값 씀) | **필수** | 포인터로 결과 반환 시 |
| `@param[in,out]` | 입출력 파라미터 (읽고 씀) | **필수** | 읽고 쓰는 포인터 |
| `@return` | 반환값 일반 설명 | **필수** | `esp_err_t` 설명 시 |
| `@retval` | 특정 반환값과 의미 명시 | 권장 | `@return` 대신 또는 병행 |

#### `@param[in]` / `@param[out]` / `@param[in,out]` — 왜 방향을 표시하는가?

**포인터 파라미터의 의도를 명확하게 하기 위함이다.**

포인터 파라미터를 받는 함수를 호출할 때, 다음 질문이 생긴다:
- 이 포인터는 입력일까, 출력일까?
- 함수가 읽기만 할까, 쓸까?
- 함수가 호출자의 메모리를 수정할까?

**방향 없이 `@param out_value 결과값`이라고만 쓰면** → 호출자가 구현 코드를 열어봐야만 알 수 있다.

**방향을 명시하면** → IDE 툴팁과 Doxygen HTML이 한눈에 알려준다.

**각 방향의 의미:**

- `@param[in]`: 함수가 이 값을 **읽기만** 한다
  - 호출자는 값을 준비해서 전달하면 된다
  - 함수가 포인터를 역참조하여 읽는다

  ```c
  // 예: 배열 내용 분석
  esp_err_t analyze_array(const int *arr);  // in 파라미터 (배열 수정 안 함)
  ```

- `@param[out]`: 함수가 이 포인터에 **결과를 쓴다**
  - 호출자가 미리 메모리를 할당해야 한다
  - 함수가 포인터를 역참조하여 값을 쓴다

  ```c
  // 예: 측정값을 저장할 포인터
  esp_err_t sensor_read(int *out_value);  // out 파라미터 (호출자가 할당 필요)
  // 호출 시: int result; sensor_read(&result);
  ```

- `@param[in,out]`: 함수가 값을 **읽은 후 수정**한다
  - 호출자의 초기값도 중요하고, 함수 실행 후 값이 변한다
  - 순서 정렬 같은 in-place 알고리즘에서 사용

  ```c
  // 예: 배열을 정렬 (배열 자체를 수정)
  void sort_array(int *arr, int len);  // in,out 파라미터
  ```

**IDE·Doxygen에서의 표시:**

- VSCode Clangd: 호버 툴팁에 `[in]` / `[out]` 배지 표시
- Doxygen HTML: "Parameters" 표에 방향 명시

**잘못된 예 vs 올바른 예:**

```c
// ❌ 잘못된 예 — 방향 불명확
/**
 * @brief ADC 센서값을 읽는다.
 * @param out_value 센서값을 저장할 포인터
 * @return ESP_OK on success
 */
esp_err_t sensor_adc_read(int *out_value);
// 호출자: "out_value가 출력 포인터라는 걸 어떻게 알지?" → 코드를 열어본다

// ✅ 올바른 예 — 방향 명확
/**
 * @brief ADC 센서값을 읽는다.
 * @param[out] out_value 센서값을 저장할 포인터 (호출자가 할당 필요)
 * @return ESP_OK on success
 */
esp_err_t sensor_adc_read(int *out_value);
// 호출자: "[out]을 봤으니, int 메모리를 준비하고 &를 붙여서 호출해야겠다" → 바로 알 수 있음
```

### 관계 및 참조 태그

| 태그 | 용도 | 필수 여부 | 비고 |
|---|---|---|---|
| `@see` | 관련 함수/파일 참조 링크 생성 | 권장 | See Also 섹션 생성 |
| `@relates` | 함수를 특정 구조체에 연결 | 선택 | 클래스 유사 그룹화 |
| `@defgroup` | 모듈 그룹 정의 (보통 `.h` 상단) | 권장 | 파일 많을수록 유용 |
| `@ingroup` | 정의된 그룹에 함수 포함 | 권장 | `@defgroup`과 쌍으로 |
| `@addtogroup` | 기존 그룹에 항목 추가 | 선택 | 분산된 파일에서 사용 |

### 코드 예시 태그

| 태그 | 용도 | 필수 여부 | 비고 |
|---|---|---|---|
| `@code` / `@endcode` | C 코드 예시 블록 | 권장 | 구문 강조 포함 |
| `\dot` / `\enddot` | Graphviz 데이터 흐름도 | 권장 | 흐름도 삽입 시 사용 |
| `@include` | 외부 파일 내용을 예시로 삽입 | 선택 | 예제 파일 참조 시 |

### 버전 및 이슈 추적 태그

| 태그 | 용도 | 필수 여부 | 비고 |
|---|---|---|---|
| `@version` | 현재 버전 표기 | 선택 | 파일 헤더에 사용 |
| `@author` | 작성자 | 선택 | 파일 헤더에 사용 |
| `@date` | 작성/수정 날짜 | 선택 | `YYYY-MM-DD` 형식 권장 |
| `@bug` | 알려진 버그 기술 | 권장 | Doxygen이 버그 목록 페이지 자동 생성 |
| `@todo` | 향후 작업 계획 | 권장 | Doxygen이 TODO 목록 페이지 자동 생성 |
| `@deprecated` | 더 이상 사용 비권장, 대안 제시 | 선택 | 대체 함수명을 함께 명시 |
| `@since` | 해당 기능이 추가된 버전 | 선택 | 버전 관리 프로젝트에 유용 |

---

## 7. 문서 구조 태그 — @mainpage / @page

Doxygen HTML의 **표지(Main Page)** 와 **주제별 독립 페이지(Related Pages)** 를 만드는 태그다.
코드 주석이 "각 함수의 설명"이라면, 이 태그들은 "문서 전체의 목차와 챕터" 역할을 한다.

| 태그 | Doxygen HTML 출력 위치 | 역할 |
|---|---|---|
| `@mainpage` | Main Page | 문서 전체의 표지/진입점 |
| `@page` | Related Pages | 주제별 독립 문서 페이지 |
| `@subpage` | Main Page 또는 `@page` 내부 | 페이지 간 계층 연결 (링크 생성) |
| `@section` | 각 페이지 내부 | 페이지 안의 소제목 구분 |

### @mainpage — 문서 표지

프로젝트 전체의 진입점이다. 하나의 프로젝트에 하나만 존재한다.
`mainpage.dox` 파일로 분리하거나, `README.md`를 재활용하는 두 가지 방법이 있다.

**방법 1: 별도 `.dox` 파일로 작성 (권장)**

```c
/* docs/mainpage.dox */

/**
 * @mainpage ESP32 센서 펌웨어
 *
 * @section intro 프로젝트 소개
 * I2C/SPI 기반 멀티센서 제어 펌웨어입니다.
 * ESP32-S3 기준으로 작성되었습니다.
 *
 * @section pages 문서 목차
 * - @subpage hardware_setup  — 하드웨어 설정 및 결선도
 * - @subpage getting_started — 개발환경 설정 및 Quick Start
 * - @subpage changelog       — 버전별 변경 이력
 *
 * @section modules 주요 모듈
 * - @ref SensorADC  — ADC 센서 드라이버
 * - @ref MqttClient — MQTT 통신 모듈
 */
```

**방법 2: README.md를 Main Page로 재활용**

별도 `.dox` 파일 없이 `README.md`를 그대로 표지로 사용한다.
문서 이중 작성을 방지할 수 있어 간단한 프로젝트에 적합하다.

```ini
# Doxyfile 설정
USE_MDFILE_AS_MAINPAGE = README.md
INPUT                 += README.md
```

> **주의:** 두 방법을 동시에 쓰면 충돌한다. 하나만 선택할 것.

### @page — 주제별 독립 페이지

Main Page에서 다루기엔 긴 내용을 별도 페이지로 분리한다.
하드웨어 설정, Quick Start, 변경 이력 등에 적합하다.

```c
/* docs/hardware_setup.dox */

/**
 * @page hardware_setup 하드웨어 설정
 *
 * @section wiring 결선도
 * | ESP32 핀 | 센서 핀 | 설명       |
 * |----------|---------|------------|
 * | GPIO 21  | SDA     | I2C 데이터 |
 * | GPIO 22  | SCL     | I2C 클럭   |
 *
 * @section power 전원 요구사항
 * 센서는 3.3V 단일 전원으로 구동됩니다.
 */
```

### 권장 디렉터리 구조

`.dox` 파일은 소스코드와 분리해 `docs/` 폴더에 모아 관리한다.

```
project/
├── src/
├── include/
└── docs/
    ├── mainpage.dox       ← @mainpage
    ├── hardware_setup.dox ← @page
    ├── getting_started.dox
    └── changelog.dox
```

Doxyfile의 `INPUT`에 `docs/` 경로를 추가해야 Doxygen이 인식한다.

```ini
INPUT = src/ include/ docs/
FILE_PATTERNS = *.c *.h *.dox
```

---

## 8. 모듈 그룹화 — @defgroup / @ingroup

파일이 여러 개로 나뉘어도 같은 모듈로 묶어서 Doxygen 출력에 정리된다.

```c
/* sensor_adc.h 상단 — 그룹 정의 */

/**
 * @defgroup SensorADC ADC 센서 드라이버
 * @brief    ADC 기반 센서 읽기 모듈 전체 API
 * @{
 */

/**
 * @brief   ADC 채널을 초기화한다.
 * @ingroup SensorADC
 */
esp_err_t sensor_adc_init(...);

/**
 * @brief   센서값을 읽는다.
 * @ingroup SensorADC
 */
esp_err_t sensor_adc_read(...);

/** @} */  // SensorADC 그룹 끝
```

---

## 9. 파일별 작성 우선순위 요약

| 항목 | `.h` 파일 | `.c` 파일 | `main.c` | `.dox` 파일 |
|---|---|---|---|---|
| 파일 헤더 | **필수** (사용법 중심) | **필수** (구현 중심) | **필수** (부팅 흐름도 포함) | 해당 없음 |
| `@brief` | **필수** | **필수** | **필수** | 해당 없음 |
| `@param[in/out]` | **필수** | 불필요 (중복) | 없음 (파라미터 없음) | 해당 없음 |
| `@return` / `@retval` | **필수** | 불필요 (중복) | 없음 (반환값 없음) | 해당 없음 |
| `@note` / `@warning` | **필수** | 선택 | 선택 | 해당 없음 |
| `@details` + `\dot` (Graphviz) | 선택 | 권장 (복잡한 로직) | 권장 (태스크 구조) | 해당 없음 |
| `@defgroup` / `@ingroup` | 권장 | 선택 | 선택 | 해당 없음 |
| `@bug` / `@todo` | 권장 | 권장 | 선택 | 해당 없음 |
| `@code` 예시 | 권장 | 선택 | 선택 | 해당 없음 |
| `@author` / `@date` | 파일 헤더에 | 파일 헤더에 | 파일 헤더에 | 해당 없음 |
| `@mainpage` | 해당 없음 | 해당 없음 | 해당 없음 | **필수** (1개만) |
| `@page` / `@section` | 해당 없음 | 해당 없음 | 해당 없음 | 권장 (주제별) |
| `@subpage` | 해당 없음 | 해당 없음 | 해당 없음 | 권장 (목차 구성) |

---

## 10. 커밋 전 체크리스트

새 함수를 추가하거나 기존 함수를 수정할 때 확인한다.

- [ ] `.h` 파일의 함수 선언에 `@brief` 작성 완료
- [ ] 파라미터가 있다면 `@param[in/out]` 모두 기술
- [ ] 반환값이 있다면 `@return` 또는 `@retval` 기술
- [ ] 하드웨어 제약이 있다면 `@note` 또는 `@warning` 추가
- [ ] 복잡한 함수라면 `.c`에 `@details` + Graphviz(`\dot`) 흐름도 추가
- [ ] 알려진 문제는 `@bug`, 향후 계획은 `@todo`로 기록
- [ ] VSCode에서 마우스 오버 툴팁이 의도대로 표시되는지 확인
- [ ] `docs/mainpage.dox`에 신규 모듈이 반영되어 있는지 확인

---

## 11. 태그 빠른 참조 카드

```
/* ───────────── 필수 ───────────── */
@brief          한 줄 요약
@param[in]      입력 파라미터
@param[out]     출력 파라미터
@return         반환값 설명

/* ───────────── 권장 ───────────── */
@note           주의사항
@warning        심각한 경고
@see            관련 함수 링크
@code           코드 예시
@bug            알려진 버그
@todo           향후 작업

/* ──────── 모듈화 시 ────────────── */
@defgroup       그룹 정의
@ingroup        그룹에 포함

/* ──────── 문서 구조 (.dox) ──────── */
@mainpage       문서 표지 (프로젝트당 1개)
@page           주제별 독립 페이지
@subpage        페이지 간 계층 연결
@section        페이지 내 소제목

/* ──────── 데이터 흐름도 ─────────── */
@details
\dot
digraph Example {
    node [shape=box];
    A [label="입력"]; B [label="처리"]; C [label="출력"];
    A -> B -> C;
}
\enddot
```

---

## 문서 정보

| 항목      | 내용       |
|-----------|-----------|
| 버전      | 1.3       |
| 작성자    | 홍길동    |
| 최초 작성 | 2025-03-05 |
| 최종 수정 | 2026-03-09 |

**변경 이력:**
- **v1.3** (2026-03-09): §3 main.c 예시를 실제 코드 구조(파일 헤더 텍스트 목록 + 함수 주석 Graphviz)로 수정, 파일 헤더 vs 함수 주석 역할 구분 설명 추가
- **v1.2** (2026-03-09): `@param[in/out/in,out]` 방향 표시의 목적과 이유를 입문자 관점에서 상세 설명 추가, IDE/Doxygen 렌더링 예시 코드 추가
- **v1.1** (2026-03-09): 동일 버전 내 추가 개선 (방향 태그 설명 추가)
- **v1.0** (2025-03-05): 초본 작성
