#include "relay_ctrl.h"
#include "exio.h"
#include "esp_log.h"

static const char *TAG = "RELAY";

// ─── 릴레이 Active 레벨 정의 ─────────────────────────────────────────────────
// TCA9554 → Optocoupler → Relay
// 회로도에 따라 Active HIGH / LOW 가 다를 수 있음.
// Waveshare 보드는 일반적으로 HIGH = ON (릴레이 활성화)
// 만약 반대로 동작하면 아래 값을 0으로 변경하세요.
#define RELAY_ACTIVE_LEVEL  1

/**
 * @brief 릴레이 초기 상태 설정 (구현 상세)
 *
 * @note RELAY_ACTIVE_LEVEL == 0 (Active Low) 인 경우 전체 핀을 HIGH로 초기화한다.
 *       RELAY_ACTIVE_LEVEL == 1 (Active High, 기본값) 인 경우 exio_init()의
 *       기본 LOW 상태 그대로이므로 별도 조작 없이 반환한다.
 *       파라미터/반환값 명세는 relay_ctrl.h 참조.
 */
esp_err_t relay_init(void)
{
    // exio_init()에서 이미 전체 LOW로 초기화됨.
    // ACTIVE_LEVEL이 0인 경우(Active Low) 전체 HIGH로 초기화 필요.
#if (RELAY_ACTIVE_LEVEL == 0)
    ESP_ERROR_CHECK(exio_write_port(0xFF));
#endif
    ESP_LOGI(TAG, "All relays initialized to OFF");
    return ESP_OK;
}

/**
 * @brief 개별 릴레이 on/off (구현 상세)
 *
 * @details
 * Active Level 변환 후 EXIO → I2C → TCA9554 → 릴레이 하드웨어 경로:
 *
 * \dot
 * digraph RelaySet {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     HW  [label="TCA9554 output pin\n-> optocoupler -> relay", style=filled, fillcolor=lightgray];
 *     err [label="ESP_ERR_INVALID_ARG"];
 *
 *     input  [label="relay_id_t id\nbool on"];
 *     valid  [label="id < RELAY_MAX?", shape=diamond];
 *     level  [label="pin_val = on ?\nRELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL"];
 *     exio   [label="exio_set_pin(id, pin_val)"];
 *     rmw    [label="비트 세트/클리어\ns_output_cache"];
 *     i2c    [label="i2c_master_transmit()\n[REG_OUTPUT, cache] -> TCA9554"];
 *
 *     input -> valid;
 *     valid -> err  [label="no"];
 *     valid -> level [label="yes"];
 *     level -> exio -> rmw -> i2c -> HW;
 * }
 * \enddot
 *
 * @note 파라미터/반환값 명세는 relay_ctrl.h 참조.
 */
esp_err_t relay_set(relay_id_t id, bool on)
{
    if (id >= RELAY_MAX) {
        ESP_LOGE(TAG, "Invalid relay id: %d", id);
        return ESP_ERR_INVALID_ARG;
    }

    // Active Level에 따라 실제 GPIO 값 결정
    uint8_t pin_val = on ? RELAY_ACTIVE_LEVEL : !RELAY_ACTIVE_LEVEL;

    ESP_LOGI(TAG, "RELAY_%d -> %s", id + 1, on ? "ON" : "OFF");
    return exio_set_pin((uint8_t)id, pin_val);
}

/**
 * @brief 특정 릴레이의 현재 상태 조회 (구현 상세)
 *
 * @details
 * EXIO 출력 레지스터에서 해당 비트를 읽어 RELAY_ACTIVE_LEVEL과 비교한다.
 * - RELAY_ACTIVE_LEVEL == 1: 비트가 HIGH이면 ON
 * - RELAY_ACTIVE_LEVEL == 0: 비트가 LOW이면 ON (비트 값 반전하여 해석)
 *
 * @note 파라미터/반환값 명세는 relay_ctrl.h 참조.
 */
bool relay_get(relay_id_t id)
{
    if (id >= RELAY_MAX) return false;

    uint8_t port = exio_get_port();
    uint8_t bit  = (port >> (uint8_t)id) & 0x01;

    // Active Level에 따라 ON/OFF 해석
    return (bit == RELAY_ACTIVE_LEVEL);
}

/**
 * @brief 전체 릴레이 상태를 8비트 비트마스크로 반환 (구현 상세)
 *
 * @details
 * exio_get_port()의 raw 캐시 값을 RELAY_ACTIVE_LEVEL에 따라 변환한다.
 * - RELAY_ACTIVE_LEVEL == 1 (Active High): raw 값 그대로 반환
 * - RELAY_ACTIVE_LEVEL == 0 (Active Low): 비트 전체를 반전(~raw)하여 반환
 * 결과 비트마스크는 항상 bit=1이 릴레이 ON을 의미한다.
 *
 * @note 파라미터/반환값 명세는 relay_ctrl.h 참조.
 */
uint8_t relay_get_all(void)
{
    // 각 비트를 Active Level 기준으로 변환하여 반환
    uint8_t raw = exio_get_port();
#if (RELAY_ACTIVE_LEVEL == 1)
    return raw;          // HIGH=ON 이므로 그대로
#else
    return ~raw;         // LOW=ON 이므로 반전
#endif
}
