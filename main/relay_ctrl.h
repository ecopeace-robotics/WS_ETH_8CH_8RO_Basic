/**
 * @file relay_ctrl.h
 * @brief exio.h 위에 구축된 고수준 릴레이 제어 API.
 *        릴레이 ID (RELAY_1..RELAY_8)를 TCA9554 출력 비트에 매핑한다.
 */

#pragma once

/**
 * @defgroup RelayControl 릴레이 제어 모듈
 * @brief TCA9554 IO 익스팬더 기반 8채널 릴레이 제어
 * @ingroup HardwareDrivers
 * @see EXIO
 * @{
 */

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief TCA9554 출력 비트에 매핑된 릴레이 채널 식별자.
 *        RELAY_1은 비트 0 (EXIO1)에, RELAY_8은 비트 7 (EXIO8)에 대응한다.
 */
typedef enum {
    RELAY_1 = 0,   /**< EXIO1 — 비트 0 */
    RELAY_2,       /**< EXIO2 — 비트 1 */
    RELAY_3,       /**< EXIO3 — 비트 2 */
    RELAY_4,       /**< EXIO4 — 비트 3 */
    RELAY_5,       /**< EXIO5 — 비트 4 */
    RELAY_6,       /**< EXIO6 — 비트 5 */
    RELAY_7,       /**< EXIO7 — 비트 6 */
    RELAY_8,       /**< EXIO8 — 비트 7 */
    RELAY_MAX      /**< 경계값 (사용 금지) */
} relay_id_t;

/**
 * @brief 릴레이 제어 초기화 (exio_init() 이후 호출)
 *        전체 릴레이 OFF 상태로 시작
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t relay_init(void);

/**
 * @brief 개별 릴레이 on/off
 * @param[in] id  RELAY_1 ~ RELAY_8
 * @param[in] on  true=ON, false=OFF
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t relay_set(relay_id_t id, bool on);

/**
 * @brief 특정 릴레이의 현재 상태 조회
 * @param[in] id  RELAY_1 ~ RELAY_8
 * @return 릴레이가 ON이면 true, OFF이면 false
 */
bool relay_get(relay_id_t id);

/**
 * @brief 현재 전체 릴레이 상태를 8비트 비트마스크로 반환
 *        bit0 = RELAY_1 ... bit7 = RELAY_8
 * @return 8비트 비트마스크; bit0=RELAY_1 ... bit7=RELAY_8
 */
uint8_t relay_get_all(void);

/** @} */  // RelayControl 그룹 끝
