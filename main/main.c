/**
 * @file main.c
 * @brief Application entry point — initialises all subsystems in order
 *        and hands control to the FreeRTOS scheduler.
 *
 * Initialisation sequence:
 *  1. NVS flash (Ethernet MAC storage)
 *  2. Default event loop
 *  3. esp_netif
 *  4. EXIO / TCA9554 IO expander
 *  5. Relay controller (all OFF)
 *  6. RC PWM outputs (neutral 1500 µs)
 *  7. W5500 Ethernet (TCP server auto-started on IP acquisition)
 */

#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "exio.h"
#include "relay_ctrl.h"
#include "pwm_ctrl.h"
#include "eth_init.h"

static const char *TAG = "MAIN";

/**
 * @brief Application entry point for the ESP32-S3 Ethernet Relay Controller.
 *
 * Initializes all hardware subsystems in sequence:
 * - NVS flash for MAC address storage
 * - Default event loop and networking interface
 * - I2C EXIO expander (TCA9554PWR)
 * - Relay controller (all OFF)
 * - RC PWM outputs (neutral at 1500 µs)
 * - W5500 Ethernet controller with TCP server auto-start
 *
 * @note This function is the ESP-IDF application entry point and is called
 *       by the bootloader. It initializes all subsystems and then yields
 *       control to the FreeRTOS scheduler.
 *
 * @return void (does not return; FreeRTOS scheduler takes over)
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=== ESP32-S3-ETH-8DI-8RO Relay Controller ===");

    // 1. NVS 초기화 (Ethernet MAC 주소 등 내부 사용)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 기본 이벤트 루프 생성
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 3. esp_netif 초기화
    ESP_ERROR_CHECK(esp_netif_init());

    // 4. I2C EXIO 익스팬더 초기화 (TCA9554PWR)
    ESP_ERROR_CHECK(exio_init());

    // 5. 릴레이 초기 상태 설정 (전체 OFF)
    ESP_ERROR_CHECK(relay_init());

    // 6. RC PWM 출력 초기화 (GPIO47=CH1, GPIO48=CH2, neutral 1500µs)
    ESP_ERROR_CHECK(pwm_init());

    // 7. W5500 Ethernet 초기화
    //    → IP 획득 시 내부에서 tcp_server_start() 자동 호출
    ESP_ERROR_CHECK(eth_init());

    ESP_LOGI(TAG, "Init complete. Waiting for Ethernet IP...");

    // 이후 FreeRTOS 스케줄러가 태스크를 관리
    // app_main은 종료되어도 무방
}