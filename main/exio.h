/**
 * @file exio.h
 * @brief ESP-IDF v5 New I2C Master API (버스 핸들 + 디바이스 핸들 모델)를
 *        사용하는 TCA9554PWR I2C IO 익스팬더 드라이버.
 *        EXIO1–EXIO8 릴레이 채널에 매핑된 8개 출력 핀을 제어한다.
 */

#pragma once
#include "esp_err.h"
#include <stdint.h>

/**
 * @brief TCA9554PWR I2C IO 익스팬더 초기화 (New I2C Master API)
 *        - I2C 버스 및 디바이스 핸들 생성
 *        - 전체 8핀을 OUTPUT으로 설정 (Config reg = 0x00)
 *        - 전체 핀 LOW로 초기화
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t exio_init(void);

/**
 * @brief 리소스 해제 (필요 시 호출)
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t exio_deinit(void);

/**
 * @brief 특정 핀의 출력값 설정
 * @param[in] pin   핀 번호 (0~7, EXIO1~8 대응)
 * @param[in] value 0=LOW, 1=HIGH
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t exio_set_pin(uint8_t pin, uint8_t value);

/**
 * @brief 8비트 비트마스크로 전체 포트 한번에 쓰기
 * @param[in] bitmask bit0=EXIO1 ... bit7=EXIO8
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t exio_write_port(uint8_t bitmask);

/**
 * @brief 현재 Output 레지스터 값 읽기 (캐시된 값)
 * @return 현재 8비트 출력 레지스터 값 (캐시된 값, 하드웨어에서 직접 읽지 않음)
 */
uint8_t exio_get_port(void);
