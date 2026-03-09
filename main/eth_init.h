/**
 * @file eth_init.h
 * @brief W5500 SPI 이더넷 초기화 — SPI 버스 설정, W5500 드라이버 설치,
 *        esp_netif 구성, IP 주소 획득 시 TCP 서버 자동 시작.
 */

#pragma once

/**
 * @defgroup EthInit W5500 이더넷 초기화
 * @brief SPI 기반 W5500 Ethernet 드라이버 초기화 및 DHCP 설정
 * @ingroup HardwareDrivers
 * @see TCPServer
 * @{
 */

#include "esp_err.h"

/**
 * @brief W5500 SPI 이더넷 초기화
 *        - SPI 버스 및 W5500 드라이버 설치
 *        - esp_netif + DHCP 설정
 *        - IP 획득 시 이벤트로 tcp_server_start() 자동 호출
 * @return 성공 시 ESP_OK, 실패 시 ESP_ERR_*
 */
esp_err_t eth_init(void);

/** @} */  // EthInit 그룹 끝
