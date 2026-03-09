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
 * @brief µs 값을 16비트 LEDC duty 값으로 변환
 *        duty = pulse_us * 65536 / PWM_PERIOD_US
 */
static uint32_t us_to_duty(uint16_t pulse_us)
{
    // 14비트 해상도 → 20000 µs에 걸쳐 16384 스텝
    return (uint32_t)pulse_us * (1U << 14) / PWM_PERIOD_US;
}

// ─── Public API ──────────────────────────────────────────────────────────────

/**
 * @brief LEDC 타이머 및 2채널 RC PWM 초기화 (구현 상세)
 *
 * @details
 * LEDC 타이머 1개 + 채널 2개 설정 흐름:
 *
 * \dot
 * digraph PwmInit {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     HW [label="LEDC HW\n(GPIO47, GPIO48)", style=filled, fillcolor=lightgray];
 *
 *     pwm_init -> "ledc_timer_config()\n50Hz, 14-bit, TIMER_1";
 *     "ledc_timer_config()\n50Hz, 14-bit, TIMER_1" -> "ledc_channel_config()\nCH1: GPIO47";
 *     "ledc_channel_config()\nCH1: GPIO47" -> "ledc_channel_config()\nCH2: GPIO48";
 *     "ledc_channel_config()\nCH2: GPIO48" -> HW [label="neutral 1500µs"];
 * }
 * \enddot
 *
 * @note 파라미터/반환값 명세는 pwm_ctrl.h 참조.
 */
esp_err_t pwm_init(void)
{
    // ── 1. LEDC 타이머 설정 ─────────────────────────────────────────────────
    ledc_timer_config_t timer_cfg = {
        .speed_mode      = PWM_LEDC_MODE,
        .duty_resolution = PWM_RESOLUTION,        // 16비트
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

/**
 * @brief 지정 채널의 펄스 폭 설정 (구현 상세)
 *
 * @details
 * µs 값 → duty 변환 → LEDC 하드웨어 반영 흐름:
 *
 * \dot
 * digraph PwmSetUs {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     HW  [label="LEDC HW output\n(GPIO47 or GPIO48)", style=filled, fillcolor=lightgray];
 *     err [label="ESP_ERR_INVALID_ARG"];
 *
 *     input   [label="channel (1|2)\npulse_us (µs)"];
 *     valid   [label="channel in {1,2}?",    shape=diamond];
 *     clamp   [label="클램프\n[1000, 2000]µs"];
 *     duty    [label="us_to_duty()\npulse_us x 16384 / 20000"];
 *     set     [label="ledc_set_duty()"];
 *     update  [label="ledc_update_duty()"];
 *     cache   [label="s_pulse_us[idx] = pulse_us"];
 *
 *     input  -> valid;
 *     valid  -> err   [label="no"];
 *     valid  -> clamp [label="yes"];
 *     clamp  -> duty -> set -> update -> cache -> HW;
 * }
 * \enddot
 *
 * @note 파라미터/반환값 명세는 pwm_ctrl.h 참조.
 */
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
    ESP_LOGI(TAG, "CH%d -> %d µs (duty=%lu)", channel, pulse_us, (unsigned long)duty);
    return ESP_OK;
}

/**
 * @brief 지정 채널의 현재 펄스 폭 반환 (구현 상세)
 *
 * @note 하드웨어를 직접 읽지 않고 s_pulse_us[] 캐시 배열에서 반환한다.
 *       채널 범위 초과 시 0 반환.
 *       파라미터/반환값 명세는 pwm_ctrl.h 참조.
 */
uint16_t pwm_get_us(uint8_t channel)
{
    if (channel < 1 || channel > 2) return 0;
    return s_pulse_us[channel - 1];
}
