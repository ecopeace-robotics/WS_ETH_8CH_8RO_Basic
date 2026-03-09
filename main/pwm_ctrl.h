/**
 * @file pwm_ctrl.h
 * @brief 두 모터 채널을 위한 LEDC 기반 RC PWM 드라이버.
 *        RC ESC / 서보 인터페이스(예: Cytron MDDS30) 호환
 *        1000–2000 µs 펄스 폭 범위에서 50 Hz 신호를 생성한다.
 */

#pragma once

/**
 * @defgroup PWMControl RC PWM 드라이버
 * @brief LEDC 기반 RC PWM 신호 생성 (50 Hz, 2채널)
 * @ingroup HardwareDrivers
 * @{
 */

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief LEDC 타이머 및 2채널 RC PWM 출력 초기화
 *        50 Hz, 14비트, 양 채널 1500 µs (중립) 로 시작
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t pwm_init(void);

/**
 * @brief 지정 채널의 펄스 폭 설정
 * @param[in] channel  1 또는 2
 * @param[in] pulse_us 펄스 폭 (µs), 범위 1000~2000 (범위 초과시 클램프)
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t pwm_set_us(uint8_t channel, uint16_t pulse_us);

/**
 * @brief 지정 채널의 현재 펄스 폭 반환 (µs)
 * @param[in] channel  1 또는 2
 * @return 현재 펄스 폭 (마이크로초, 1000–2000)
 */
uint16_t pwm_get_us(uint8_t channel);

/** @} */  // PWMControl 그룹 끝
