#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    RELAY_1 = 0,   // EXIO1 (bit 0)
    RELAY_2,       // EXIO2 (bit 1)
    RELAY_3,       // EXIO3 (bit 2)
    RELAY_4,       // EXIO4 (bit 3)
    RELAY_5,       // EXIO5 (bit 4)
    RELAY_6,       // EXIO6 (bit 5)
    RELAY_7,       // EXIO7 (bit 6)
    RELAY_8,       // EXIO8 (bit 7)
    RELAY_MAX
} relay_id_t;

/**
 * @brief 릴레이 제어 초기화 (exio_init() 이후 호출)
 *        전체 릴레이 OFF 상태로 시작
 */
esp_err_t relay_init(void);

/**
 * @brief 개별 릴레이 on/off
 * @param id  RELAY_1 ~ RELAY_8
 * @param on  true=ON, false=OFF
 */
esp_err_t relay_set(relay_id_t id, bool on);

/**
 * @brief 특정 릴레이의 현재 상태 조회
 */
bool relay_get(relay_id_t id);

/**
 * @brief 현재 전체 릴레이 상태를 8비트 마스크로 반환
 *        bit0 = RELAY_1 ... bit7 = RELAY_8
 */
uint8_t relay_get_all(void);