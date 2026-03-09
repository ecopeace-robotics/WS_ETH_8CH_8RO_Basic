/**
 * @file main.c
 * @brief 애플리케이션 진입점 — 모든 서브시스템을 순서대로 초기화하고
 *        FreeRTOS 스케줄러에 제어를 넘긴다.
 *
 * 초기화 순서:
 *  1. NVS 플래시 (Ethernet MAC 주소 저장)
 *  2. 기본 이벤트 루프
 *  3. esp_netif
 *  4. EXIO / TCA9554 IO 익스팬더
 *  5. 릴레이 컨트롤러 (전체 OFF)
 *  6. RC PWM 출력 (중립 1500 µs)
 *  7. W5500 이더넷 (IP 획득 시 TCP 서버 자동 시작)
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
 * @brief ESP32-S3 이더넷 릴레이 컨트롤러 애플리케이션 진입점.
 *
 * @details
 * 모든 하드웨어 서브시스템을 순서대로 초기화한 뒤 FreeRTOS 스케줄러에
 * 제어를 넘긴다. TCP 서버는 Ethernet IP 획득 이벤트 수신 후 자동 시작된다.
 *
 * \dot
 * digraph BootFlow {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     Bootloader      [style=filled, fillcolor=lightgray];
 *     Scheduler       [label="FreeRTOS Scheduler", style=filled, fillcolor=lightgray];
 *     tcp_server_task [label="tcp_server_task\n(FreeRTOS task)", style=filled, fillcolor=lightyellow];
 *
 *     Bootloader -> nvs_flash_init;
 *     nvs_flash_init -> esp_event_loop_create_default;
 *     esp_event_loop_create_default -> esp_netif_init;
 *     esp_netif_init -> exio_init;
 *     exio_init -> relay_init;
 *     relay_init -> pwm_init;
 *     pwm_init -> eth_init;
 *     eth_init -> Scheduler [label="반환"];
 *     eth_init -> tcp_server_task [label="IP_EVENT_ETH_GOT_IP\n(비동기, 최초 1회)", style=dashed];
 * }
 * \enddot
 *
 * @note 이 함수는 ESP-IDF 애플리케이션 진입점으로 부트로더에 의해 호출된다.
 *       반환되지 않으며 FreeRTOS 스케줄러가 제어를 이어받는다.
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
