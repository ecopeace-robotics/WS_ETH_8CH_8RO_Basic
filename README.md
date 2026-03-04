# ESP32-S3 Ethernet Motor & Relay Controller

> W5500 SPI 이더넷 기반 TCP 명령으로 8채널 릴레이 및 2채널 RC PWM을 제어하는 ESP32-S3 펌웨어

- **보드**: Waveshare ESP32-S3-ETH-8DI-8RO
- **프레임워크**: ESP-IDF (CMake)
- **최초 작성일**: 2025-01-01
- **API 문서**: [Doxygen → GitHub Pages](https://ecopeace-robotics.github.io/WS_ETH_8CH_8RO_Basic/)

---

## 목차

1. [소개](#소개)
2. [하드웨어 구성](#하드웨어-구성)
3. [소프트웨어 의존성](#소프트웨어-의존성)
4. [Quick Start](#quick-start)
5. [프로젝트 구조](#프로젝트-구조)
6. [코드 구동 흐름](#코드-구동-흐름)
7. [TCP 제어 프로토콜](#tcp-제어-프로토콜)
8. [주요 함수 및 API](#주요-함수-및-api)
9. [알려진 이슈 및 주의사항](#알려진-이슈-및-주의사항)
10. [변경 이력](#변경-이력)
11. [참고](#참고)

---

## 소개

이 펌웨어는 **Waveshare ESP32-S3-ETH-8DI-8RO** 보드를 위한 네트워크 제어 펌웨어입니다.

- W5500 SPI 이더넷 칩을 통해 TCP 소켓으로 연결
- TCA9554 I2C IO 익스팬더로 8채널 릴레이 ON/OFF 제어
- ESP32-S3 LEDC로 2채널 RC PWM 신호 출력 (모터드라이버 인터페이스용)
- 고정 IP 또는 DHCP 선택 가능
- TCP 텍스트 프로토콜(포트 8080)으로 모든 기능 원격 제어

---

## 하드웨어 구성

### 주요 부품

| 부품 | 역할 |
|------|------|
| ESP32-S3 | 메인 MCU (FreeRTOS) |
| W5500 | SPI 이더넷 컨트롤러 |
| TCA9554PWR | I2C → 8-bit IO 익스팬더 (릴레이 구동) |
| 8채널 릴레이 | 부하 개폐 |
| Cytron MDDS30 (선택) | RC PWM 입력 듀얼 모터드라이버 |

### 핀 연결

**W5500 SPI 이더넷**

| 신호 | ESP32-S3 GPIO |
|------|--------------|
| MOSI | GPIO 13 |
| MISO | GPIO 14 |
| SCLK | GPIO 15 |
| CS   | GPIO 16 |
| INT  | GPIO 12 |
| RST  | GPIO 39 |
| SPI Clock | 20 MHz (SPI2_HOST) |

**TCA9554 I2C IO 익스팬더 (릴레이)**

| 신호 | ESP32-S3 GPIO | 비고 |
|------|--------------|------|
| SCL  | GPIO 41 | 100 kHz |
| SDA  | GPIO 42 | I2C 주소 0x20 (A0=A1=A2=GND) |

**RC PWM 출력 (LEDC)**

| 채널 | GPIO | 사양 |
|------|------|------|
| PWM CH1 | GPIO 47 | 50 Hz, 1000–2000 µs, 14-bit |
| PWM CH2 | GPIO 48 | 50 Hz, 1000–2000 µs, 14-bit |

**네트워크 기본값**

| 항목 | 값 |
|------|-----|
| IP 모드 | Static (변경 가능: `ETH_USE_STATIC_IP = 0` → DHCP) |
| IP 주소 | 192.168.1.100 |
| 서브넷 | 255.255.255.0 |
| 게이트웨이 | 192.168.1.1 |
| TCP 포트 | 8080 |

---

## 소프트웨어 의존성

| 항목 | 버전 |
|------|------|
| ESP-IDF | v5.x 권장 |
| 개발 OS | Ubuntu 22.04 / macOS |
| 추가 컴포넌트 | 없음 (모두 ESP-IDF 내장) |

---

## Quick Start

```bash
# 1. 저장소 클론
git clone https://github.com/ecopeace-robotics/WS_ETH_8CH_8RO_Basic.git
cd WS_ETH_8CH_8RO_Basic

# 2. ESP-IDF 환경 활성화 (설치 경로에 맞게 조정)
. $HOME/esp/esp-idf/export.sh

# 3. 빌드 및 플래시
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

> IP/네트워크 설정 변경: `main/board_config.h` 수정 후 재빌드

**동작 확인 (telnet)**

```bash
telnet 192.168.1.100 8080
RELAY 1 ON
RELAY 1 OFF
STATUS
PWM 1 1500
PWM STATUS
```

---

## 프로젝트 구조

```
WS_ETH_8CH_8RO_Basic/
├── main/
│   ├── board_config.h   # GPIO/네트워크/PWM 컴파일 상수 (여기서 설정 변경)
│   ├── main.c           # app_main() 진입점, 초기화 순서 정의
│   ├── eth_init.c/.h    # W5500 SPI 이더넷 초기화, IP 이벤트 처리
│   ├── exio.c/.h        # TCA9554 I2C IO 익스팬더 저수준 드라이버
│   ├── relay_ctrl.c/.h  # 릴레이 고수준 API (번호 기반 ON/OFF)
│   ├── pwm_ctrl.c/.h    # LEDC RC PWM 드라이버 (µs 단위 제어)
│   ├── tcp_server.c/.h  # TCP 서버 태스크, 텍스트 명령 파서
│   └── CMakeLists.txt
├── Doxyfile             # Doxygen 설정
├── CMakeLists.txt
└── README.md
```

---

## 코드 구동 흐름

```
app_main()
├── 1. nvs_flash_init()       — Ethernet MAC 주소 저장소 초기화
├── 2. esp_event_loop_create_default()
├── 3. esp_netif_init()
├── 4. exio_init()            — TCA9554 I2C 초기화, 전체 핀 출력 모드 설정
├── 5. relay_ctrl_init()      — 릴레이 전체 OFF 초기화
├── 6. pwm_ctrl_init()        — LEDC 타이머 설정, 양 채널 1500 µs (중립) 출력 시작
└── 7. eth_init_start()       — W5500 SPI 설정, IP 획득 시 tcp_server_start() 자동 호출
        └── IP 획득 이벤트
                └── tcp_server_start()  — FreeRTOS 태스크 생성, 포트 8080 리슨
```

---

## TCP 제어 프로토콜

포트 **8080**, 텍스트 명령, `\n` 종료

### 클라이언트 → 서버

| 명령 | 예시 | 설명 |
|------|------|------|
| `RELAY <n> ON\n` | `RELAY 1 ON` | 릴레이 n번 (1~8) ON |
| `RELAY <n> OFF\n` | `RELAY 2 OFF` | 릴레이 n번 (1~8) OFF |
| `STATUS\n` | `STATUS` | 모든 릴레이 상태 조회 |
| `PWM <ch> <us>\n` | `PWM 1 1700` | PWM ch(1 or 2)에 펄스폭 us(1000~2000) 설정 |
| `PWM STATUS\n` | `PWM STATUS` | 양 채널 현재 펄스폭 조회 |

### 서버 → 클라이언트

| 응답 | 의미 |
|------|------|
| `OK` | 명령 성공 |
| `ERROR` | 잘못된 명령 |
| `R1=ON R2=OFF ...` | STATUS 응답 |
| `PWM1=1500 PWM2=1500` | PWM STATUS 응답 |

---

## 주요 함수 및 API

> 상세 함수 문서: [Doxygen API Reference](https://ecopeace-robotics.github.io/WS_ETH_8CH_8RO_Basic/)

| 함수 | 파일 | 설명 |
|------|------|------|
| `eth_init_start()` | eth_init.c | W5500 이더넷 초기화 및 이벤트 핸들러 등록 |
| `relay_ctrl_init()` | relay_ctrl.c | TCA9554 기반 릴레이 초기화 (전체 OFF) |
| `relay_ctrl_set(n, state)` | relay_ctrl.c | 릴레이 n번 ON/OFF 제어 |
| `pwm_ctrl_init()` | pwm_ctrl.c | LEDC 타이머·채널 초기화, 1500 µs 출력 시작 |
| `pwm_set_us(ch, us)` | pwm_ctrl.c | 채널 ch에 펄스폭 us(µs) 설정 |
| `pwm_get_us(ch)` | pwm_ctrl.c | 채널 ch 현재 펄스폭 반환 |
| `tcp_server_start()` | tcp_server.c | TCP 서버 FreeRTOS 태스크 생성 |

---

## 알려진 이슈 및 주의사항

### 주의사항
- `board_config.h`의 `ETH_STATIC_IP`를 네트워크 환경에 맞게 수정 후 빌드할 것
- PWM 펄스폭은 1000~2000 µs 범위 외 값 입력 시 `ERROR` 응답
- 릴레이 번호는 1~8 (0 또는 9 이상 입력 시 `ERROR`)

### TODO
- [ ] DHCP 환경에서 IP 변경 시 TCP 서버 재시작 처리
- [ ] 8채널 DI(디지털 입력) 읽기 기능 구현

---

## 변경 이력

| 날짜 | 버전 | 내용 |
|------|------|------|
| 2025-01-01 | v1.0 | 최초 작성 — 릴레이 8CH + PWM 2CH + TCP 제어 |

---

## 참고

- [ESP-IDF 공식 문서](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [W5500 데이터시트](https://docs.wiznet.io/Product/iEthernet/W5500/datasheet)
- [TCA9554 데이터시트](https://www.ti.com/lit/ds/symlink/tca9554.pdf)
- [Cytron MDDS30 사용 가이드](https://www.cytron.io/p-mdds30)
- [Waveshare ESP32-S3-ETH-8DI-8RO 위키](https://www.waveshare.com/wiki/ESP32-S3-ETH-8DI-8RO)
