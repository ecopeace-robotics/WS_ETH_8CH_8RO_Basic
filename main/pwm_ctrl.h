#pragma once

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief LEDC 타이머 및 2채널 RC PWM 출력 초기화
 *        50 Hz, 16-bit, 양 채널 1500 µs (neutral) 로 시작
 */
esp_err_t pwm_init(void);

/**
 * @brief 지정 채널의 펄스 폭 설정
 * @param channel  1 또는 2
 * @param pulse_us 펄스 폭 (µs), 범위 1000~2000 (범위 초과시 클램프)
 */
esp_err_t pwm_set_us(uint8_t channel, uint16_t pulse_us);

/**
 * @brief 지정 채널의 현재 펄스 폭 반환 (µs)
 * @param channel  1 또는 2
 */
uint16_t pwm_get_us(uint8_t channel);
