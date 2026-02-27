# Eth_Motor — ESP32-S3 이더넷 모터 컨트롤러

ESP32-S3 기반 릴레이 보드를 이용해 유선 이더넷으로 DC 모터 2개를 제어하는 프로젝트입니다.
ESP32-S3가 TCP 서버 역할을 하며, 같은 네트워크에 있는 PC에서 텍스트 명령을 보내 제어합니다.

---

## 목차

1. [프로젝트 개요](#1-프로젝트-개요)
2. [하드웨어](#2-하드웨어)
3. [소프트웨어 아키텍처](#3-소프트웨어-아키텍처)
4. [프로젝트 파일 구조](#4-프로젝트-파일-구조)
5. [모듈별 설명](#5-모듈별-설명)
6. [TCP 명령 프로토콜](#6-tcp-명령-프로토콜)
7. [네트워크 설정](#7-네트워크-설정)
8. [빌드 및 플래시](#8-빌드-및-플래시)
9. [sdkconfig 및 파티션 설정](#9-sdkconfig-및-파티션-설정)
10. [키보드 제어 도구](#10-키보드-제어-도구)
11. [트러블슈팅](#11-트러블슈팅)

---

## 1. 프로젝트 개요

```
[ PC / 노트북 ]  ─── 이더넷 ───  [ ESP32-S3 보드 ]  ─── PWM 신호 ───  [ MDDS30 모터 드라이버 ]  ─── DC 모터 2개 ]
                                         │
                                      I2C 버스
                                         │
                                  [ TCA9554 IO 익스팬더 ]
                                         │
                                  [ 릴레이 8채널 출력 ]
```

- ESP32-S3는 **W5500 SPI 이더넷 칩**을 통해 LAN에 연결됩니다.
- **TCP 서버(포트 8080)**를 열고 텍스트 명령을 기다립니다.
- ESP32의 **LEDC 주변 장치**가 50 Hz RC PWM 신호(1000–2000 µs)를 2채널 생성하여 **MDDS30 모터 드라이버**에 공급합니다.
- 온보드 릴레이 8개는 **TCA9554PWR I2C IO 익스팬더**를 통해 제어되며 TCP 명령으로 ON/OFF할 수 있습니다.

---

## 2. 하드웨어

### MCU 보드 — Waveshare ESP32-S3-ETH-8DI-8RO

| 기능 | 내용 |
|---|---|
| MCU | ESP32-S3 |
| 이더넷 | W5500 SPI 칩 |
| IO 익스팬더 | TCA9554PWR (I2C, 릴레이용 8채널 출력) |
| 릴레이 출력 | 8채널 |
| 디지털 입력 | 8채널 (DI, 현재 펌웨어에서 미사용) |

### 모터 드라이버 — Cytron MDDS30

RC PWM(RC 서보 방식) 신호를 입력받는 듀얼채널 30 A DC 모터 드라이버입니다.

| PWM 펄스 폭 | 모터 상태 |
|---|---|
| 1000 µs | 최대 역방향 |
| 1500 µs | 정지(뉴트럴) |
| 2000 µs | 최대 정방향 |

전체 설정 방법은 [Docs/MDDS30_Howto.md](./Docs/MDDS30_Howto.md)를 참고하세요.

### GPIO 핀 할당

| 신호 | GPIO | 설명 |
|---|---|---|
| ETH MOSI | 13 | W5500으로 SPI 데이터 송신 |
| ETH MISO | 14 | W5500으로부터 SPI 데이터 수신 |
| ETH SCLK | 15 | SPI 클럭 |
| ETH CS | 16 | W5500 칩 선택 |
| ETH INT | 12 | W5500 인터럽트 (현재 미사용, 폴링 모드) |
| ETH RST | 39 | W5500 하드웨어 리셋 |
| I2C SDA | 42 | TCA9554 데이터 |
| I2C SCL | 41 | TCA9554 클럭 |
| PWM CH1 | 47 | RC PWM → MDDS30 모터 1 (왼쪽) |
| PWM CH2 | 48 | RC PWM → MDDS30 모터 2 (오른쪽) |

---

## 3. 소프트웨어 아키텍처

### 시작 순서 (`app_main`)

```
app_main()
  │
  ├─ nvs_flash_init()          ← 비휘발성 스토리지 초기화 (이더넷 MAC 주소 등에 사용)
  ├─ esp_event_loop_create()   ← FreeRTOS 이벤트 버스 생성 (WiFi/ETH/IP 이벤트 처리)
  ├─ esp_netif_init()          ← TCP/IP 스택 초기화
  │
  ├─ exio_init()               ← I2C + TCA9554 IO 익스팬더 초기화
  ├─ relay_init()              ← 릴레이 8개 전체 OFF 초기화
  ├─ pwm_init()                ← LEDC 타이머 + PWM 2채널 뉴트럴(1500 µs) 출력 시작
  │
  └─ eth_init()                ← W5500 SPI 이더넷 드라이버 + IP 이벤트 핸들러 등록
          │
          └─ (이벤트: IP_EVENT_ETH_GOT_IP — IP 주소 획득 시)
                  │
                  └─ tcp_server_start()   ← FreeRTOS TCP 리슨 태스크 생성
```

`app_main`이 반환된 이후에도 **FreeRTOS 스케줄러**가 시스템을 유지합니다. 아래 세 종류의 태스크가 동시에 동작합니다.

| 태스크 | 스택 | 우선순위 | 역할 |
|---|---|---|---|
| `tcp_server_task` | 4 KB | 5 | TCP 연결 수락 루프 |
| `tcp_client` (연결당 1개) | 4 KB | 5 | 클라이언트별 명령 처리 |
| 이더넷 드라이버 내부 태스크 | 내부 관리 | 다양 | W5500 폴링 / 링크 관리 |

### FreeRTOS 태스크를 쓰는 이유

임베디드 펌웨어는 네트워크 데이터를 기다리는 동안 CPU를 블로킹할 수 없습니다 — PWM 신호 생성, 이더넷 처리 등 다른 작업이 있기 때문입니다. FreeRTOS는 각 태스크가 `recv()` 같은 블로킹 호출에서 잠드는 동안 다른 태스크가 계속 실행될 수 있게 해줍니다. 태스크는 경량 스레드라고 생각하면 됩니다.

---

## 4. 프로젝트 파일 구조

```
eth_motor/
├── CMakeLists.txt          ← 최상위 ESP-IDF 빌드 스크립트 (프로젝트 이름 정의)
├── partitions.csv          ← 플래시 파티션 레이아웃 (NVS, OTA, 앱, SPIFFS)
├── sdkconfig               ← ESP-IDF Kconfig 설정값 (빌드 시 자동 생성)
│
├── main/                   ← 모든 애플리케이션 소스 코드
│   ├── CMakeLists.txt      ← 소스 파일 목록 및 필요한 ESP-IDF 컴포넌트 선언
│   ├── board_config.h      ← ★ 모든 하드웨어 상수 (GPIO, IP, 포트, 타이밍)
│   ├── main.c              ← 진입점: 순서대로 모든 모듈 초기화
│   ├── eth_init.c/.h       ← W5500 SPI 이더넷 드라이버 + 이벤트 처리
│   ├── exio.c/.h           ← TCA9554PWR I2C IO 익스팬더 드라이버
│   ├── relay_ctrl.c/.h     ← 릴레이 제어 고수준 API (exio 위에서 동작)
│   ├── pwm_ctrl.c/.h       ← LEDC 기반 RC PWM 출력 (2채널)
│   └── tcp_server.c/.h     ← TCP 서버 + 명령 파서
│
├── tools/
│   └── keyboard_ctrl.py    ← PC용 키보드 제어 스크립트 (Python)
│
└── Docs/
    └── MDDS30_Howto.md     ← 모터 드라이버 설정 가이드
```

> **초보자 팁:** 읽기 시작하기 좋은 순서는 `board_config.h`(핀 번호 한곳에 모아둠) → `main.c`(시작 순서) → 관심 있는 모듈 순입니다.

---

## 5. 모듈별 설명

### `board_config.h` — 하드웨어 상수 정의

프로젝트 전체에서 사용하는 모든 하드웨어 숫자의 단일 출처입니다. 배선을 바꾸거나 IP 주소를 변경할 때는 **이 파일만 수정**하면 됩니다.

주요 정의:
- `ETH_*_GPIO` — W5500 이더넷 칩의 SPI 핀
- `EXIO_SCL_GPIO` / `EXIO_SDA_GPIO` — 릴레이 IO 익스팬더의 I2C 핀
- `ETH_USE_STATIC_IP` / `ETH_STATIC_IP` — `0`이면 DHCP, `1`이면 고정 IP 사용
- `PWM_CH1_GPIO` / `PWM_CH2_GPIO` — PWM 출력 핀
- `PWM_US_MIN/MAX/NEUTRAL` — RC 서보 펄스 범위 (1000–2000 µs, 뉴트럴 1500 µs)
- `TCP_SERVER_PORT` — TCP 서버 포트 번호 (기본값 8080)

---

### `eth_init.c` — 이더넷 초기화

**W5500을 SPI로 연결**하고 ESP-IDF의 TCP/IP 스택(lwIP)에 붙이는 작업을 합니다.

단계별 동작:
1. W5500용 `netif`(네트워크 인터페이스) 객체를 생성합니다.
2. SPI2 버스를 초기화합니다 (`spi_bus_initialize`).
3. W5500 MAC 및 PHY 드라이버 객체를 생성합니다.
4. ESP32 내장 MAC 주소를 읽어 W5500에 기록합니다 — **이 단계가 핵심**입니다. 없으면 W5500의 MAC이 `00:00:00:00:00:00`이 되어 DHCP/ARP가 동작하지 않습니다.
5. netif를 이더넷 드라이버에 연결합니다.
6. `ETHERNET_EVENT` 및 `IP_EVENT` 이벤트 핸들러를 등록합니다.
7. 이더넷 드라이버를 시작합니다.

이 보드에서 인터럽트 모드가 링크 불안정을 일으키는 것이 확인되어 W5500을 **폴링 모드**(`poll_period_ms = 10`)로 구성했습니다.

IP 주소를 얻으면 `ip_event_handler` 콜백이 자동으로 `tcp_server_start()`를 호출합니다 — TCP 서버는 단 한 번만 시작됩니다.

---

### `exio.c` — TCA9554PWR IO 익스팬더 드라이버

보드의 릴레이 8개 출력은 ESP32 GPIO에 직접 연결되지 않습니다. I2C 버스에 연결된 **TCA9554PWR**(주소 `0x20`)이라는 8비트 IO 익스팬더를 통해 제어합니다.

이 모듈은 ESP-IDF 5.x의 **새 I2C Master API**(`i2c_master_bus_handle_t` / `i2c_master_dev_handle_t`)를 사용합니다. 버스와 디바이스를 분리 관리하여 나중에 I2C 주변 기기를 추가하기 쉽습니다.

주요 함수:
- `exio_init()` — I2C 버스 생성, 디바이스 등록, 8핀 전체를 출력으로 설정, 전체 LOW 초기화
- `exio_set_pin(pin, value)` — 8개 출력 핀 중 하나를 HIGH 또는 LOW로 설정
- `exio_write_port(bitmask)` — 8핀 전체를 1바이트 마스크로 한 번에 설정
- `exio_get_port()` — 캐시된 출력 상태 반환 (I2C 읽기 없음)

**Read-modify-write 패턴:** TCA9554는 "특정 비트 하나만 토글" 명령이 없습니다. 따라서 드라이버가 마지막 쓰기 값의 로컬 복사본(`s_output_cache`)을 유지합니다. 한 비트를 바꿀 때 캐시를 업데이트하고 전체 바이트를 다시 씁니다.

---

### `relay_ctrl.c` — 릴레이 제어 레이어

`exio` 위에 얹힌 얇은 추상화 계층입니다. 릴레이 번호(1–8, `relay_id_t` 열거형)를 IO 익스팬더 핀으로 매핑합니다.

`RELAY_ACTIVE_LEVEL` 정의로 극성을 조정합니다:
- `1` = HIGH가 릴레이를 활성화 (기본값, 옵토커플러 방식 보드에서 일반적)
- `0` = LOW가 릴레이를 활성화 — 릴레이가 반대로 동작할 경우 이 값을 바꾸세요

주요 함수:
- `relay_init()` — 모든 릴레이를 OFF 상태로 초기화
- `relay_set(id, on)` — 릴레이 하나를 ON 또는 OFF
- `relay_get(id)` — 특정 릴레이의 현재 ON/OFF 상태 반환
- `relay_get_all()` — 전체 릴레이 상태를 8비트 마스크로 반환

---

### `pwm_ctrl.c` — RC PWM 출력

ESP32의 **LEDC**(LED Control) 주변 장치를 사용해 50 Hz RC 서보 PWM 신호를 2채널 생성합니다. 이름은 LED 제어지만 실제로는 범용 PWM 발생기입니다.

**왜 50 Hz / 1000–2000 µs인가?**
RC 취미 전자기기(ESC, MDDS30 같은 서보 컨트롤러)의 보편 표준입니다. 한 주기는 20 ms이고, 하이 펄스가 1 ms(최대 역방향)~2 ms(최대 정방향), 1.5 ms가 정지입니다.

LEDC는 **14비트 해상도**로 설정되어 20 ms 주기에 걸쳐 16384 단계를 제공합니다. 내부 헬퍼 함수 `us_to_duty(pulse_us)`가 마이크로초 값을 LEDC duty 레지스터 정수로 변환합니다.

주요 함수:
- `pwm_init()` — LEDC 타이머와 두 채널을 설정, 뉴트럴 출력으로 시작
- `pwm_set_us(channel, pulse_us)` — 채널의 펄스 폭을 설정 (1000–2000 µs 범위로 클램프)
- `pwm_get_us(channel)` — 캐시된 현재 펄스 폭(µs) 반환

---

### `tcp_server.c` — TCP 서버 및 명령 파서

포트 8080에서 TCP 연결을 수신 대기하고 텍스트 명령을 처리합니다.

**두 개의 FreeRTOS 태스크:**

1. **`tcp_server_task`** — 리스너. `accept()`를 루프로 호출하며, 클라이언트가 연결될 때마다 새 `client_task`를 생성합니다.
2. **`client_task`** — 연결당 1개. 소켓에서 데이터를 링 버퍼로 읽고, `\n` 기준으로 줄을 분리한 뒤 공백/CR을 제거하고 완성된 줄마다 `handle_command()`를 호출합니다.

**명령 파서(`handle_command`):**
`sscanf`와 `strncmp`로 수신 텍스트를 알려진 명령 패턴과 비교해 `relay_set()` 또는 `pwm_set_us()`로 라우팅합니다.

클라이언트 연결 시 웰컴 배너를 전송합니다:
```
ESP32 Relay Controller ready. Commands: RELAY <1-8> ON|OFF, STATUS
```

---

## 6. TCP 명령 프로토콜

`nc`, Python `socket`, 또는 아래의 키보드 제어 도구 등 어떤 TCP 클라이언트로도 연결할 수 있습니다.

모든 명령은 일반 ASCII 텍스트이며 개행 문자(`\n`)로 끝납니다.

### 릴레이 명령

| 명령 | 예시 | 응답 |
|---|---|---|
| `RELAY <1–8> ON` | `RELAY 3 ON` | `OK` |
| `RELAY <1–8> OFF` | `RELAY 3 OFF` | `OK` |
| `STATUS` | `STATUS` | `R1=OFF R2=ON R3=OFF ...` |

### PWM 모터 명령

| 명령 | 예시 | 응답 |
|---|---|---|
| `PWM <1\|2> <1000–2000>` | `PWM 1 1800` | `OK` |
| `PWM STATUS` | `PWM STATUS` | `PWM1=1500 PWM2=1500` |

### 오류 응답

| 응답 | 의미 |
|---|---|
| `ERROR: relay number must be 1~8` | 릴레이 번호 범위 초과 |
| `ERROR: state must be ON or OFF` | 상태 문자열 오타 |
| `ERROR: PWM channel must be 1 or 2` | 잘못된 PWM 채널 번호 |
| `ERROR: pulse width must be 1000~2000 µs` | 펄스 폭 범위 초과 |
| `ERROR: unknown command` | 인식할 수 없는 명령 |

### `nc`(netcat)으로 빠른 테스트

```bash
# PC에서 실행 (ESP32 IP가 192.168.1.100인 경우)
nc 192.168.1.100 8080

# 연결 후 명령 입력:
RELAY 1 ON
STATUS
PWM 1 1800
PWM 2 1200
PWM STATUS
```

---

## 7. 네트워크 설정

모든 네트워크 설정은 [main/board_config.h](main/board_config.h)에 있습니다:

```c
#define ETH_USE_STATIC_IP   1               // 1 = 고정 IP, 0 = DHCP
#define ETH_STATIC_IP       "192.168.1.100"
#define ETH_STATIC_MASK     "255.255.255.0"
#define ETH_STATIC_GW       "192.168.1.1"

#define TCP_SERVER_PORT     8080
```

DHCP를 사용하려면 `ETH_USE_STATIC_IP`를 `0`으로 바꾸고 다시 빌드하세요. 할당된 IP는 시리얼 모니터에서 확인할 수 있습니다.

---

## 8. 빌드 및 플래시

### 사전 준비

- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/)
- Python 3.x (ESP-IDF에 포함)

### 빌드 및 플래시

```bash
# 1. ESP-IDF 환경 활성화
. $IDF_PATH/export.sh       # Linux/macOS

# 2. 타겟 칩 설정
idf.py set-target esp32s3

# 3. 빌드
idf.py build

# 4. 플래시 (/dev/ttyUSB0는 실제 포트로 교체)
idf.py -p /dev/ttyUSB0 flash

# 5. 시리얼 모니터 (종료: Ctrl+])
idf.py -p /dev/ttyUSB0 monitor
```

### 플래시 파티션 구성

[partitions.csv](partitions.csv)에 정의되어 있습니다:

| 이름 | 타입 | 크기 | 용도 |
|---|---|---|---|
| nvs | data | 20 KB | 키-값 스토리지 (MAC 주소 등) |
| otadata | data | 8 KB | OTA 업데이트 관리 |
| app0 | app | 3 MB | 애플리케이션 펌웨어 |
| spiffs | data | ~10 MB | 파일 시스템 (향후 사용 예약) |

---

## 9. sdkconfig 및 파티션 설정

### sdkconfig란?

`sdkconfig`는 ESP-IDF의 Kconfig 빌드 시스템이 생성하는 **프로젝트 전체 설정 파일**입니다. `idf.py menuconfig`를 실행하면 터미널 UI가 열리고, 여기서 값을 바꾸면 `sdkconfig`가 자동으로 갱신됩니다. 이 파일은 빌드 시 `sdkconfig.h`로 변환되어 C 코드에서 `#ifdef CONFIG_...` 형태로 참조됩니다.

> **주의:** `sdkconfig`는 직접 텍스트 에디터로 편집하지 않는 것을 권장합니다. 의존 관계가 복잡하므로 반드시 `idf.py menuconfig`를 사용하세요.

---

### 이 프로젝트의 핵심 sdkconfig 항목

#### 1. W5500 SPI 이더넷 드라이버 활성화

```
Component config → Ethernet
```

| 항목 | 설정값 | 설명 |
|---|---|---|
| `CONFIG_ETH_ENABLED` | `y` | 이더넷 드라이버 전체 활성화 |
| `CONFIG_ETH_USE_SPI_ETHERNET` | `y` | SPI 방식 외부 이더넷 칩 지원 활성화 |
| `CONFIG_ETH_SPI_ETHERNET_W5500` | `y` | W5500 칩 드라이버 포함 |

W5500을 사용하려면 이 세 항목이 모두 `y`여야 합니다. 하나라도 비활성화하면 `esp_eth_new_spi_eth_w5500()` 함수가 링커에서 찾을 수 없다는 오류가 발생합니다.

#### 2. 커스텀 파티션 테이블 사용

```
Partition Table → Partition Table → Custom partition table CSV
```

| 항목 | 설정값 | 설명 |
|---|---|---|
| `CONFIG_PARTITION_TABLE_CUSTOM` | `y` | 기본 파티션 레이아웃 대신 CSV 파일 사용 |
| `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` | `"partitions.csv"` | 참조할 CSV 파일 경로 |

기본 파티션 레이아웃은 앱 크기가 약 1 MB로 제한됩니다. 이 프로젝트는 3 MB 앱 파티션을 사용하므로 반드시 커스텀 테이블이 필요합니다.

#### 3. 플래시 크기 및 속도

```
Serial flasher config
```

| 항목 | 설정값 | 설명 |
|---|---|---|
| `CONFIG_ESPTOOLPY_FLASHSIZE` | `"16MB"` | 보드 탑재 플래시 크기 — 실제 칩과 일치해야 함 |
| `CONFIG_ESPTOOLPY_FLASHFREQ` | `"80m"` | 플래시 통신 속도 80 MHz |
| `CONFIG_ESPTOOLPY_FLASHMODE` | `"dio"` | Dual I/O 모드 |

플래시 크기 설정이 실제 칩보다 크면 플래시 범위를 벗어난 파티션에 쓰기를 시도해 부팅 실패가 발생합니다. 반드시 보드 스펙을 확인하세요.

#### 4. SPI ISR IRAM 배치 (W5500 안정성)

```
Component config → Driver configurations → SPI Configuration
```

| 항목 | 설정값 | 설명 |
|---|---|---|
| `CONFIG_SPI_MASTER_ISR_IN_IRAM` | `y` | SPI 마스터 인터럽트 핸들러를 IRAM에 배치 |

플래시 캐시가 비워지는 순간(플래시 쓰기, OTA 등)에도 SPI ISR이 계속 실행될 수 있도록 합니다. W5500처럼 지속적으로 폴링/인터럽트를 사용하는 SPI 이더넷에 특히 중요합니다.

#### 5. ESP32-S3 범용 MAC 주소 풀

```
Component config → ESP32S3-Specific
```

| 항목 | 설정값 | 설명 |
|---|---|---|
| `CONFIG_ESP32S3_UNIVERSAL_MAC_ADDRESSES_FOUR` | `y` | MAC 주소 4개 풀 사용 (WiFi STA, WiFi AP, BT, ETH) |

이 설정이 활성화되어 있어야 `esp_read_mac(mac, ESP_MAC_ETH)`가 이더넷 전용 고유 MAC 주소를 반환합니다. 비활성화하면 MAC 주소 풀이 2개로 줄어들어 이더넷 MAC이 WiFi MAC과 겹칠 수 있습니다.

#### 6. PSRAM (SPIRAM) 활성화

```
Component config → ESP PSRAM
```

| 항목 | 설정값 | 설명 |
|---|---|---|
| `CONFIG_SPIRAM` | `y` | 외부 PSRAM 활성화 |
| `CONFIG_SPIRAM_MODE_OCT` | `y` | Octal SPI 모드 (이 보드 PSRAM 타입에 맞게 설정) |
| `CONFIG_SPIRAM_USE_MALLOC` | `y` | `malloc()`이 내부 메모리 부족 시 PSRAM도 사용 |

이 보드에 PSRAM이 탑재되어 있으므로 활성화해 둡니다. 비활성화해도 현재 펌웨어는 동작하지만, 향후 SPIFFS 파일 시스템이나 대용량 버퍼를 사용할 때 내부 메모리가 부족해질 수 있습니다.

---

### partitions.csv — 플래시 파티션 레이아웃 상세

16 MB 플래시를 아래와 같이 분할합니다:

```
플래시 주소 맵 (16 MB = 0x1000000)
┌─────────────────────────────────────────────────────────┐
│ 0x0000  부트로더 (ESP-IDF 내장, ~32 KB)                 │
│ 0x8000  파티션 테이블 (partitions.csv 컴파일 결과)       │
├─────────────────────────────────────────────────────────┤
│ 0x9000  nvs      │ 20 KB  │ NVS 키-값 스토리지           │
│ 0xE000  otadata  │  8 KB  │ OTA 상태 메타데이터           │
│ 0x10000 app0     │  3 MB  │ 애플리케이션 펌웨어           │
│ 0x310000 spiffs  │ ~10 MB │ SPIFFS 파일 시스템 (예약)     │
└─────────────────────────────────────────────────────────┘
```

#### 각 파티션 설명

**`nvs` (Non-Volatile Storage, 20 KB)**

ESP-IDF의 키-값 스토리지 라이브러리가 사용하는 영역입니다. 전원이 꺼져도 데이터가 유지됩니다. 이 프로젝트에서는 주로 이더넷 드라이버 내부와 `nvs_flash_init()`이 자동으로 사용합니다. 크기를 줄이면 저장 가능한 항목 수가 줄어들고, 너무 작으면 `nvs_flash_init()` 실패로 부팅이 안 됩니다.

**`otadata` (OTA Data, 8 KB)**

OTA(Over-The-Air) 업데이트 기능을 위한 메타데이터 파티션입니다. 현재 어떤 앱 파티션(`ota_0` / `ota_1`)에서 부팅해야 하는지 기록합니다. 현재 이 프로젝트는 OTA 슬롯이 `app0` 하나뿐이라 실질적으로 사용되지 않지만, `ota_0` 타입 앱과 함께 사용하면 향후 무선 펌웨어 업데이트를 추가할 수 있습니다.

**`app0` (애플리케이션, 3 MB)**

컴파일된 펌웨어 바이너리가 저장되는 곳입니다. `idf.py flash` 실행 시 `.bin` 파일이 이 파티션에 쓰입니다. 3 MB는 ESP-IDF 기본값(1 MB)보다 크며, ETH + lwIP + FreeRTOS + LEDC 등 여러 컴포넌트를 포함해도 충분합니다. 펌웨어가 이 크기를 초과하면 빌드 시 오류가 납니다.

**`spiffs` (SPIFFS 파일 시스템, ~10 MB)**

현재 펌웨어에서는 사용하지 않지만 향후 확장을 위해 예약된 영역입니다. 여기에는 웹 UI용 HTML/CSS/JS 파일, 설정 파일(JSON), 로그 파일 등을 저장할 수 있습니다. SPIFFS를 활성화하려면 `CMakeLists.txt`에 `spiffs` 컴포넌트를 추가하고 `esp_vfs_spiffs_register()`로 마운트하면 됩니다.

#### 파티션 테이블 수정 시 주의사항

- **오프셋은 4 KB(0x1000) 단위로 정렬**해야 합니다.
- **전체 파티션 크기 합계**가 `CONFIG_ESPTOOLPY_FLASHSIZE`를 초과하면 안 됩니다.
- 파티션 테이블을 변경한 후에는 **전체 플래시를 지우고 다시 플래시**해야 합니다. 레이아웃 불일치 상태에서 일부만 쓰면 부팅 실패가 발생합니다:

```bash
idf.py -p /dev/ttyUSB0 erase-flash
idf.py -p /dev/ttyUSB0 flash
```

---

## 10. 키보드 제어 도구

[tools/keyboard_ctrl.py](tools/keyboard_ctrl.py)는 TCP를 통해 키보드로 로버를 운전할 수 있는 Python 스크립트입니다. 표준 라이브러리만 사용하므로 추가 패키지 설치가 필요 없습니다.

### 사용법

```bash
# 기본 IP(192.168.0.197) 사용
python tools/keyboard_ctrl.py

# IP 직접 지정
python tools/keyboard_ctrl.py 192.168.1.100
```

### 키 바인딩

| 키 | 동작 |
|---|---|
| `W` / `↑` | 전진 |
| `S` / `↓` | 후진 |
| `A` / `←` | 제자리 좌회전 |
| `D` / `→` | 제자리 우회전 |
| `W`+`A` / `W`+`D` | 곡선 이동 (한쪽 모터 빠르게) |
| `Space` | 긴급 정지 (양 채널 → 1500 µs) |
| `Q` / `Esc` | 종료 (종료 전 모터 정지) |

### 동작 원리

스크립트는 터미널을 **raw 모드**로 전환하여 Enter 없이 개별 키 입력 이벤트를 읽습니다. 키를 누른 채 있으면 터미널이 약 30 ms마다 키 이벤트를 반복 전송하는데, 이를 이용해 키가 "눌려 있는" 상태를 추적합니다. 간단한 램프 처리(`RAMP_STEP = 20 µs/틱`)를 적용하여 속도가 급격히 변하지 않고 부드럽게 변합니다.

왼쪽 모터는 **PWM CH1**, 오른쪽 모터는 **PWM CH2**에 해당하며, 표준 탱크 드라이브 믹싱 공식을 사용합니다:

```
왼쪽  = NEUTRAL + 스로틀 * 500 - 회전 * 500
오른쪽 = NEUTRAL + 스로틀 * 500 + 회전 * 500
```

---

## 11. 트러블슈팅

### W5500 — Link Up은 되는데 IP를 못 받는 경우

아래 순서대로 확인하세요.

**1. MAC 주소 미설정 (가장 흔한 원인)**

W5500은 외부 SPI 칩이므로 ESP32 내장 MAC 주소를 자동으로 사용하지 않습니다. MAC을 명시적으로 기록하지 않으면 W5500이 `00:00:00:00:00:00`으로 송신하며, 스위치/라우터가 해당 프레임을 조용히 드롭합니다. DHCP DISCOVER 패킷이 라우터에 도달하지 못하므로 IP를 절대 받을 수 없습니다. 이 코드가 `eth_init.c`에 이미 적용되어 있습니다:

```c
uint8_t mac_addr[6];
ESP_ERROR_CHECK(esp_read_mac(mac_addr, ESP_MAC_ETH));
ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr));
```

이 코드를 다른 프로젝트에 복사할 때 이 세 줄을 빠뜨리지 마세요.

**2. `esp_netif_init()` 이중 호출**

`app_main()`에서 정확히 한 번만 호출해야 합니다. `eth_init()` 내부에서 다시 호출하면 ESP-IDF 5.x에서 `ESP_ERR_INVALID_STATE` abort가 발생합니다.

**3. GPIO ISR 서비스 미설치 (인터럽트 모드 사용 시)**

인터럽트 모드(`int_gpio_num`에 실제 GPIO 설정)로 전환할 경우 W5500 MAC 생성 전에 반드시 호출해야 합니다:

```c
gpio_install_isr_service(0);  // W5500 MAC 생성 이전에 호출
```

누락 시 로그에 아래 오류가 출력되며 인터럽트 핸들러 등록에 실패합니다:
```
E gpio: gpio_isr_handler_add: GPIO isr service is not installed
```

**4. 인터럽트 모드 링크 불안정 (Link Up → Link Down 반복)**

인터럽트 모드에서 링크가 불안정한 경우 폴링 모드로 전환합니다 (이미 적용됨):

```c
w5500_cfg.int_gpio_num   = -1;   // 인터럽트 비활성화
w5500_cfg.poll_period_ms = 10;   // 10ms마다 링크 상태 폴링
```

### 릴레이가 반대로 동작하는 경우 (OFF 명령에 ON)

[main/relay_ctrl.c](main/relay_ctrl.c)의 `RELAY_ACTIVE_LEVEL`을 `1`에서 `0`으로 변경하세요.

### 모터가 반대 방향으로 회전하는 경우

제어 스크립트에서 `PWM 1`과 `PWM 2` 명령을 바꾸거나, MDDS30 출력 단자에서 모터 배선을 물리적으로 교체하세요.

### TCP 서버에 연결이 안 되는 경우

1. 시리얼 모니터에서 `Got IP: ...` 로그가 출력됐는지 확인합니다.
2. PC가 ESP32와 같은 서브넷에 있는지 확인합니다.
3. 방화벽이 포트 8080을 차단하고 있지 않은지 확인합니다.
4. 먼저 `ping <ESP32_IP>`를 시도합니다. ping이 안 되면 TCP 레이어가 아닌 이더넷/IP 레이어 문제입니다.
