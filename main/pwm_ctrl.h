/**
 * @file pwm_ctrl.h
 * @brief LEDC-based RC PWM driver for two motor channels.
 *        Generates 50 Hz signals in the 1000–2000 µs pulse-width range
 *        compatible with RC ESC / servo interfaces (e.g. Cytron MDDS30).
 */

#pragma once

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief LEDC 타이머 및 2채널 RC PWM 출력 초기화
 *        50 Hz, 14-bit, 양 채널 1500 µs (neutral) 로 시작
 * @return ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t pwm_init(void);

/**
 * @brief 지정 채널의 펄스 폭 설정
 * @param channel  1 or 2
 * @param pulse_us 펄스 폭 (µs), 범위 1000~2000 (범위 초과시 클램프)
 * @return ESP_OK on success, ESP_ERR_* on failure
 */
esp_err_t pwm_set_us(uint8_t channel, uint16_t pulse_us);

/**
 * @brief 지정 채널의 현재 펄스 폭 반환 (µs)
 * @param channel  1 or 2
 * @return Current pulse width in microseconds (1000–2000)
 */
uint16_t pwm_get_us(uint8_t channel);
