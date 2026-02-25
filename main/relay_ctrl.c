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

bool relay_get(relay_id_t id)
{
    if (id >= RELAY_MAX) return false;

    uint8_t port = exio_get_port();
    uint8_t bit  = (port >> (uint8_t)id) & 0x01;

    // Active Level에 따라 ON/OFF 해석
    return (bit == RELAY_ACTIVE_LEVEL);
}

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