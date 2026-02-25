#pragma once
#include "esp_err.h"

/**
 * @brief W5500 SPI Ethernet 초기화
 *        - SPI 버스 및 W5500 드라이버 설치
 *        - esp_netif + DHCP 설정
 *        - IP 획득 시 이벤트로 tcp_server_start() 자동 호출
 */
esp_err_t eth_init(void);