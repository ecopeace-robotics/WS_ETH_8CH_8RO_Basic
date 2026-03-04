# [프로젝트 이름]

> 한 줄 설명 — 무엇을 하는 프로젝트인지 간단히

- **담당자**: 이름 (이메일 또는 연락처)
- **최초 작성일**: YYYY-MM-DD
- **최종 수정일**: YYYY-MM-DD

---

## 목차

1. [소개](#소개)
2. [구동 환경 및 의존성](#구동-환경-및-의존성)
3. [Quick Start](#quick-start)
4. [프로젝트 구조](#프로젝트-구조)
5. [코드 구동 흐름](#코드-구동-흐름)
6. [주요 함수 및 API](#주요-함수-및-api)
7. [사용 중인 프로젝트](#사용-중인-프로젝트)
8. [알려진 이슈 및 주의사항](#알려진-이슈-및-주의사항)
9. [변경 이력](#변경-이력)
10. [참고](#참고)

---

## 소개

프로젝트의 목적과 배경을 간략히 서술합니다.

- 왜 만들었는지
- 어떤 문제를 해결하는지
- 기존 방식과 다른 점이 있다면

---

## 구동 환경 및 의존성

### 하드웨어
| 항목 | 내용 |
|------|------|
| 보드 | ESP32-DevKitC |
| 센서 / 모듈 | (예: DHT22, OLED 등) |

### 결선도

![결선도](docs/wiring.png)
<!-- 이미지가 없을 경우 아래 핀 연결 표로 대체 -->

| 부품 | 부품 핀 | ESP32 핀 | 비고 |
|------|---------|----------|------|
| DHT22 | VCC | 3.3V | |
| DHT22 | DATA | GPIO4 | 10kΩ 풀업 저항 필요 |
| DHT22 | GND | GND | |

> 결선도 이미지는 Fritzing, KiCad 등으로 작성 후 `docs/` 폴더에 저장 권장

### 소프트웨어
| 항목 | 버전 |
|------|------|
| 프레임워크 | ESP-IDF v5.0 |
| 개발 OS | Ubuntu 22.04 / Windows 11 |
| 기타 라이브러리 | (예: FreeRTOS, LVGL 등) |

---

## Quick Start

최소한으로 돌아가는 실행 방법만 서술합니다.

```bash
# 1. 저장소 클론
git clone https://github.com/yourname/project-name.git
cd project-name

# 2. 빌드 및 플래시
idf.py build flash monitor
```

> 처음 환경 세팅이 필요하다면 [구동 환경 및 의존성](#구동-환경-및-의존성) 먼저 확인

---

## 프로젝트 구조

```
project-name/
├── src/
│   ├── main.c          # 진입점 및 전체 흐름 제어
│   ├── sensor.c        # 센서 드라이버
│   ├── sensor.h
│   └── ...
├── docs/               # 자동 생성 문서 (Doxygen) 및 결선도
├── README.md
└── Doxyfile
```

각 파일/폴더의 역할을 한 줄씩 서술합니다.

---

## 코드 구동 흐름

main 함수 기준 전체 동작 흐름을 시각화합니다.

```
app_main()
├── 1. 하드웨어 초기화 (sensor_init)
├── 2. 루프 시작
│   ├── sensor_read_temperature()
│   ├── sensor_read_humidity()
│   └── uart_print_result()
└── 3. 오류 발생 시 재시작
```

> 흐름이 복잡할 경우 별도 다이어그램 이미지 첨부 권장

---

## 주요 함수 및 API

> 상세 함수 문서는 Doxygen 자동 생성 문서 참고: [링크](https://yourname.github.io/project-name)

여기서는 핵심 함수만 간략히 기술합니다.

### `sensor_read_temperature(int gpio_pin)`

| 항목 | 내용 |
|------|------|
| 입력 | `gpio_pin` — 센서 연결 GPIO 핀 번호 |
| 출력 | `float` 섭씨 온도값. 실패 시 `-999.0` 반환 |
| 알고리즘 | DHT22 단선 통신 프로토콜로 40bit 데이터 수신 후 파싱 |
| 외부 의존 | ESP-IDF `driver/gpio.h` |

### `sensor_read_humidity(int gpio_pin)`

| 항목 | 내용 |
|------|------|
| 입력 | `gpio_pin` — 센서 연결 GPIO 핀 번호 |
| 출력 | `float` 상대습도(%). 실패 시 `-999.0` 반환 |
| 알고리즘 | `sensor_read_temperature()`와 동일 통신, 다른 바이트 파싱 |
| 외부 의존 | ESP-IDF `driver/gpio.h` |

---

## 사용 중인 프로젝트

이 코드를 현재 사용 중인 프로젝트 목록입니다.

| 프로젝트명 | 담당자 | 저장소 | 비고 |
|-----------|--------|--------|------|
| 공장 온습도 모니터링 | 홍길동 | [링크](https://github.com/yourname/project-a) | v1.1 사용 중 |
| 냉장 창고 경보 시스템 | 김철수 | [링크](https://github.com/yourname/project-b) | v1.0 사용 중, 업데이트 예정 |

> 이 코드를 새 프로젝트에 적용했다면 위 표에 추가해주세요.
> 코드 변경 시 사용 중인 프로젝트 담당자에게 사전 공유 필요

---

## 알려진 이슈 및 주의사항

### 이슈
- [ ] 연속 호출 시 2초 미만 간격이면 읽기 실패 발생 (DHT22 스펙 제한)
- [ ] 3.3V 미만 전원 공급 시 간헐적 오동작

### 주의사항 (이렇게 쓰면 안 됩니다)
- `sensor_read_temperature()`를 루프에서 딜레이 없이 반복 호출하지 말 것
- GPIO 핀 번호를 0으로 설정하면 부팅 오류 발생

### TODO
- [ ] Wi-Fi를 통한 원격 전송 기능 추가 예정
- [ ] 저전력 모드 미구현

---

## 변경 이력

| 날짜 | 버전 | 내용 | 작성자 |
|------|------|------|--------|
| 2024-03-01 | v1.0 | 최초 작성 | 홍길동 |
| 2024-03-15 | v1.1 | 습도 읽기 오류 수정 | 홍길동 |

---

## 참고

- [ESP-IDF 공식 문서](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [DHT22 데이터시트](https://cdn-shop.adafruit.com/datasheets/DHT22.pdf)
- 사용한 외부 API나 라이브러리가 있다면 이곳에 정리
