#include "eth_init.h"
#include "board_config.h"
#include "tcp_server.h"

#include "esp_log.h"
#include "esp_eth.h"
#include "esp_eth_mac_spi.h"   // W5500 MAC + PHY 모두 여기에 선언됨
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

static const char *TAG = "ETH";

static esp_eth_handle_t     s_eth_handle   = NULL;
static esp_netif_t         *s_eth_netif    = NULL;
static bool                 s_server_started = false;

// ─── 이벤트 핸들러 ────────────────────────────────────────────────────────────

/**
 * @brief Ethernet 링크/상태 이벤트 핸들러 (ETHERNET_EVENT_*)
 *        링크 UP/DOWN, Start/Stop 상태를 로그로 기록한다.
 */
static void eth_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Up");
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/**
 * @brief IP 획득 이벤트 핸들러 (IP_EVENT_ETH_GOT_IP)
 *        최초 IP 획득 시 tcp_server_start()를 한 번만 호출한다.
 */
static void ip_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        // IP 획득 후 TCP 서버 최초 1회만 시작
        if (!s_server_started) {
            s_server_started = true;
            tcp_server_start();
        }
    }
}

// ─── 공개 함수 ────────────────────────────────────────────────────────────────

/**
 * @brief W5500 SPI 이더넷 초기화 (구현 상세)
 *
 * @details
 * 초기화 단계 및 이벤트 기반 TCP 서버 시작 흐름:
 *
 * \dot
 * digraph EthInit {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     tcp_server_task [label="tcp_server_task\n(FreeRTOS task)", style=filled, fillcolor=lightyellow];
 *
 *     eth_init -> "esp_netif_new()";
 *     "esp_netif_new()" -> "spi_bus_initialize()";
 *     "spi_bus_initialize()" -> "esp_eth_mac_new_w5500()";
 *     "esp_eth_mac_new_w5500()" -> "esp_eth_phy_new_w5500()";
 *     "esp_eth_phy_new_w5500()" -> "esp_eth_driver_install()";
 *     "esp_eth_driver_install()" -> "MAC 주소 설정";
 *     "MAC 주소 설정" -> "esp_netif_attach()";
 *     "esp_netif_attach()" -> "이벤트 핸들러 등록";
 *     "이벤트 핸들러 등록" -> "esp_eth_start()";
 *     "esp_eth_start()" -> "IP_EVENT_ETH_GOT_IP" [label="비동기", style=dashed];
 *     "IP_EVENT_ETH_GOT_IP" -> tcp_server_task [label="최초 1회"];
 * }
 * \enddot
 *
 * @note 파라미터/반환값 명세는 eth_init.h 참조.
 */
esp_err_t eth_init(void)
{
    // 1. W5500용 기본 netif 생성
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    s_eth_netif = esp_netif_new(&netif_cfg);

    // 2. SPI 버스 초기화
    spi_bus_config_t buscfg = {
        .mosi_io_num   = ETH_MOSI_GPIO,
        .miso_io_num   = ETH_MISO_GPIO,
        .sclk_io_num   = ETH_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 3. W5500 SPI 디바이스 설정
    spi_device_interface_config_t devcfg = {
        .command_bits     = 16,
        .address_bits     = 8,
        .mode             = 0,
        .clock_speed_hz   = ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num     = ETH_CS_GPIO,
        .queue_size       = 20,
    };

    // 4. GPIO ISR 서비스 설치 (W5500 인터럽트 모드에 필요)
    gpio_install_isr_service(0);

    // 5. W5500 MAC 생성
    eth_w5500_config_t w5500_cfg = ETH_W5500_DEFAULT_CONFIG(ETH_SPI_HOST, &devcfg);
    w5500_cfg.int_gpio_num = -1;        // 폴링 모드로 변경 (인터럽트 비활성화)
    w5500_cfg.poll_period_ms = 10;      // 10ms 폴링 간격

    eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_cfg, &mac_cfg);

    // 6. W5500 PHY 생성
    eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
    phy_cfg.reset_gpio_num = ETH_RST_GPIO;
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_cfg);

    // 7. ETH 드라이버 설치
    esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_ERROR_CHECK(esp_eth_driver_install(&eth_cfg, &s_eth_handle));

    // 7-b. ESP32 내장 ETH MAC 주소를 W5500에 기록 (없으면 00:00:00:00:00:00 → DHCP 불가)
    uint8_t mac_addr[6];
    ESP_ERROR_CHECK(esp_read_mac(mac_addr, ESP_MAC_ETH));
    ESP_ERROR_CHECK(esp_eth_ioctl(s_eth_handle, ETH_CMD_S_MAC_ADDR, mac_addr));
    ESP_LOGI(TAG, "MAC: %02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);

    // 8. netif과 eth 드라이버 연결
    ESP_ERROR_CHECK(esp_netif_attach(s_eth_netif,
                                     esp_eth_new_netif_glue(s_eth_handle)));

    // 9. 이벤트 핸들러 등록
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                               eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                                               ip_event_handler, NULL));

    // 10. Ethernet 시작
    ESP_ERROR_CHECK(esp_eth_start(s_eth_handle));

    ESP_LOGI(TAG, "W5500 Ethernet init done, waiting for IP...");
    return ESP_OK;
}
