#include "exio.h"
#include "board_config.h"
#include "driver/i2c_master.h"   // New I2C Master API (ESP-IDF 5.x)
#include "esp_log.h"

static const char *TAG = "EXIO";

// ─── New API 핸들 ─────────────────────────────────────────────────────────────
// Legacy API: i2c_port_t (정수) 하나로 관리
// New API   : 버스 핸들 + 디바이스 핸들 분리 → 멀티 디바이스에 유연
static i2c_master_bus_handle_t  s_bus_handle = NULL;
static i2c_master_dev_handle_t  s_dev_handle = NULL;

// 현재 출력 포트 상태 캐시 (read-modify-write용)
static uint8_t s_output_cache = 0x00;

// ─── 내부 헬퍼 ───────────────────────────────────────────────────────────────

/**
 * @brief TCA9554 레지스터에 1바이트 쓰기
 *
 * New API 전송 방식:
 *   i2c_master_transmit(dev, buf, len, timeout_ms)
 *   → START + ADDR(W) + buf[0](reg) + buf[1](data) + STOP
 *   내부적으로 ACK 확인까지 처리
 */
static esp_err_t tca9554_write_reg(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { reg, data };

    esp_err_t ret = i2c_master_transmit(s_dev_handle, buf, sizeof(buf), 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write reg=0x%02X data=0x%02X failed: %s",
                 reg, data, esp_err_to_name(ret));
    }
    return ret;
}

// ─── Public API ──────────────────────────────────────────────────────────────

esp_err_t exio_init(void)
{
    // ── 1. I2C 버스 생성 ────────────────────────────────────────────────────
    // New API는 버스(bus)와 디바이스(device)를 분리하여 핸들로 관리.
    // 하나의 버스에 여러 디바이스를 attach 할 수 있음.
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port          = I2C_NUM_0,
        .sda_io_num        = EXIO_SDA_GPIO,
        .scl_io_num        = EXIO_SCL_GPIO,
        .clk_source        = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,           // 글리치 필터 (권장값 7)
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &s_bus_handle));

    // ── 2. TCA9554PWR 디바이스 attach ───────────────────────────────────────
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,  // 7비트 주소
        .device_address  = EXIO_I2C_ADDR,        // 0x20
        .scl_speed_hz    = EXIO_I2C_FREQ_HZ,     // 100kHz
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &s_dev_handle));

    // ── 3. TCA9554 초기화 ───────────────────────────────────────────────────
    // Config 레지스터 0x03: 0x00 = 전체 8핀 OUTPUT 방향
    ESP_ERROR_CHECK(tca9554_write_reg(TCA9554_REG_CONFIG, 0x00));

    // Output 레지스터 0x01: 전체 LOW (릴레이 모두 OFF)
    s_output_cache = 0x00;
    ESP_ERROR_CHECK(tca9554_write_reg(TCA9554_REG_OUTPUT, s_output_cache));

    ESP_LOGI(TAG, "TCA9554PWR initialized (New I2C API), all outputs LOW");
    return ESP_OK;
}

esp_err_t exio_deinit(void)
{
    if (s_dev_handle) {
        ESP_ERROR_CHECK(i2c_master_bus_rm_device(s_dev_handle));
        s_dev_handle = NULL;
    }
    if (s_bus_handle) {
        ESP_ERROR_CHECK(i2c_del_master_bus(s_bus_handle));
        s_bus_handle = NULL;
    }
    return ESP_OK;
}

esp_err_t exio_set_pin(uint8_t pin, uint8_t value)
{
    if (pin > 7) {
        ESP_LOGE(TAG, "Invalid pin: %d", pin);
        return ESP_ERR_INVALID_ARG;
    }
    if (value) {
        s_output_cache |=  (uint8_t)(1 << pin);
    } else {
        s_output_cache &= ~(uint8_t)(1 << pin);
    }
    return tca9554_write_reg(TCA9554_REG_OUTPUT, s_output_cache);
}

esp_err_t exio_write_port(uint8_t bitmask)
{
    s_output_cache = bitmask;
    return tca9554_write_reg(TCA9554_REG_OUTPUT, s_output_cache);
}

uint8_t exio_get_port(void)
{
    return s_output_cache;
}