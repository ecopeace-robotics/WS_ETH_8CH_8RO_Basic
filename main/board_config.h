#pragma once

// =============================================================================
// W5500 Ethernet (SPI)
// =============================================================================
#define ETH_SPI_HOST        SPI2_HOST
#define ETH_SPI_CLOCK_MHZ   20

#define ETH_MOSI_GPIO       13
#define ETH_MISO_GPIO       14
#define ETH_SCLK_GPIO       15
#define ETH_CS_GPIO         16
#define ETH_INT_GPIO        12
#define ETH_RST_GPIO        39

// =============================================================================
// TCA9554PWR - I2C IO Expander (릴레이 제어)
// A0=A1=A2=GND → 기본 주소 0x20
// =============================================================================
#define EXIO_SCL_GPIO       41
#define EXIO_SDA_GPIO       42
#define EXIO_I2C_ADDR       0x20
#define EXIO_I2C_FREQ_HZ    100000

// TCA9554 레지스터
#define TCA9554_REG_INPUT       0x00
#define TCA9554_REG_OUTPUT      0x01
#define TCA9554_REG_POLARITY    0x02
#define TCA9554_REG_CONFIG      0x03   // 0=output, 1=input

// =============================================================================
// 네트워크 설정
// ETH_USE_STATIC_IP 1 → 정적 IP 사용 (DHCP 없는 환경 테스트용)
// ETH_USE_STATIC_IP 0 → DHCP 사용
// =============================================================================
#define ETH_USE_STATIC_IP   1
#define ETH_STATIC_IP       "192.168.1.100"
#define ETH_STATIC_MASK     "255.255.255.0"
#define ETH_STATIC_GW       "192.168.1.1"

// =============================================================================
// TCP 서버
// =============================================================================
#define TCP_SERVER_PORT     8080
#define TCP_RX_BUF_SIZE     128