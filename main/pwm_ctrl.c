#include "pwm_ctrl.h"
#include "board_config.h"

#include "esp_log.h"
#include "driver/ledc.h"

static const char *TAG = "PWM";

// ─── LEDC 채널 매핑 ───────────────────────────────────────────────────────────
// CH1 → LEDC_CHANNEL_0, CH2 → LEDC_CHANNEL_1
static const ledc_channel_t s_ledc_ch[2] = {
    LEDC_CHANNEL_0,
    LEDC_CHANNEL_1,
};

static const int s_gpio[2] = {
    PWM_CH1_GPIO,
    PWM_CH2_GPIO,
};

// 현재 펄스 폭 캐시 (µs)
static uint16_t s_pulse_us[2] = { PWM_US_NEUTRAL, PWM_US_NEUTRAL };

// ─── 내부 헬퍼 ───────────────────────────────────────────────────────────────

/**
 * @brief µs 값을 16-bit LEDC duty 값으로 변환
 *        duty = pulse_us * 65536 / PWM_PERIOD_US
 */
static uint32_t us_to_duty(uint16_t pulse_us)
{
    // 14-bit resolution → 16384 steps over 20000 µs
    return (uint32_t)pulse_us * (1U << 14) / PWM_PERIOD_US;
}

// ─── Public API ──────────────────────────────────────────────────────────────

esp_err_t pwm_init(void)
{
    // ── 1. LEDC 타이머 설정 ─────────────────────────────────────────────────
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = PWM_LEDC_MODE,
        .duty_resolution = PWM_RESOLUTION,        // 16-bit
        .timer_num       = PWM_LEDC_TIMER,        // LEDC_TIMER_1
        .freq_hz         = PWM_FREQ_HZ,           // 50 Hz
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_timer_config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "LEDC timer configured: %dHz, 16-bit", PWM_FREQ_HZ);

    // ── 2. 각 채널 설정 및 neutral 출력 ─────────────────────────────────────
    for (int i = 0; i < 2; i++) {
        ledc_channel_config_t ch_cfg = {
            .speed_mode = PWM_LEDC_MODE,
            .channel    = s_ledc_ch[i],
            .timer_sel  = PWM_LEDC_TIMER,
            .intr_type  = LEDC_INTR_DISABLE,
            .gpio_num   = s_gpio[i],
            .duty       = us_to_duty(PWM_US_NEUTRAL),
            .hpoint     = 0,
        };
        ret = ledc_channel_config(&ch_cfg);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "ledc_channel_config CH%d failed: %s", i + 1, esp_err_to_name(ret));
            return ret;
        }
        s_pulse_us[i] = PWM_US_NEUTRAL;
    }

    ESP_LOGI(TAG, "CH1 GPIO%d neutral, CH2 GPIO%d neutral (%d µs)",
             PWM_CH1_GPIO, PWM_CH2_GPIO, PWM_US_NEUTRAL);
    return ESP_OK;
}

esp_err_t pwm_set_us(uint8_t channel, uint16_t pulse_us)
{
    if (channel < 1 || channel > 2) {
        ESP_LOGE(TAG, "Invalid channel: %d", channel);
        return ESP_ERR_INVALID_ARG;
    }

    // 범위 클램프
    if (pulse_us < PWM_US_MIN) pulse_us = PWM_US_MIN;
    if (pulse_us > PWM_US_MAX) pulse_us = PWM_US_MAX;

    uint32_t duty = us_to_duty(pulse_us);
    int idx = channel - 1;

    esp_err_t ret = ledc_set_duty(PWM_LEDC_MODE, s_ledc_ch[idx], duty);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_set_duty CH%d failed: %s", channel, esp_err_to_name(ret));
        return ret;
    }
    ret = ledc_update_duty(PWM_LEDC_MODE, s_ledc_ch[idx]);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ledc_update_duty CH%d failed: %s", channel, esp_err_to_name(ret));
        return ret;
    }

    s_pulse_us[idx] = pulse_us;
    ESP_LOGI(TAG, "CH%d → %d µs (duty=%lu)", channel, pulse_us, (unsigned long)duty);
    return ESP_OK;
}

uint16_t pwm_get_us(uint8_t channel)
{
    if (channel < 1 || channel > 2) return 0;
    return s_pulse_us[channel - 1];
}
