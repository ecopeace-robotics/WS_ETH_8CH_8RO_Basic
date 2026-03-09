/**
 * @file board_config.h
 * @brief Waveshare ESP32-S3-ETH-8DI-8RO 보드용 하드웨어 핀 배정 및
 *        컴파일 타임 설정 상수.
 *
 * 이 파일을 수정하여 드라이버 코드를 건드리지 않고 다른 GPIO 배정,
 * 네트워크 설정, 또는 PWM 파라미터에 펌웨어를 적응시킬 수 있습니다.
 */

#pragma once

/** @defgroup CFG_ETH Ethernet (W5500 SPI) 핀 및 클록 설정
 *  @{
 */
#define ETH_SPI_HOST        SPI2_HOST   /**< W5500에 사용하는 SPI 호스트 주변장치 */
#define ETH_SPI_CLOCK_MHZ   20          /**< SPI 클록 속도 (MHz) */

#define ETH_MOSI_GPIO       13          /**< SPI MOSI GPIO */
#define ETH_MISO_GPIO       14          /**< SPI MISO GPIO */
#define ETH_SCLK_GPIO       15          /**< SPI SCLK GPIO */
#define ETH_CS_GPIO         16          /**< W5500 칩 셀렉트 GPIO */
#define ETH_INT_GPIO        12          /**< W5500 인터럽트 GPIO */
#define ETH_RST_GPIO        39          /**< W5500 하드웨어 리셋 GPIO */
/** @} */

/** @defgroup CFG_EXIO TCA9554PWR I2C IO 익스팬더 (릴레이 제어)
 *  A0=A1=A2=GND → 기본 I2C 주소 0x20
 *  @{
 */
#define EXIO_SCL_GPIO       41          /**< I2C SCL GPIO */
#define EXIO_SDA_GPIO       42          /**< I2C SDA GPIO */
#define EXIO_I2C_ADDR       0x20        /**< 7비트 I2C 주소 (A0=A1=A2=GND) */
#define EXIO_I2C_FREQ_HZ    100000      /**< I2C 클록 주파수 (Hz) */

/** @defgroup CFG_TCA9554_REGS TCA9554 내부 레지스터 주소
 *  @{
 */
#define TCA9554_REG_INPUT       0x00    /**< 입력 포트 레지스터 (읽기 전용) */
#define TCA9554_REG_OUTPUT      0x01    /**< 출력 포트 레지스터 */
#define TCA9554_REG_POLARITY    0x02    /**< 극성 반전 레지스터 */
#define TCA9554_REG_CONFIG      0x03    /**< 설정 레지스터: 0=출력, 1=입력 */
/** @} */
/** @} */

/** @defgroup CFG_NET 네트워크 설정
 *  현재 eth_init.c는 DHCP를 사용합니다 (ESP_NETIF_DEFAULT_ETH 기본값).
 *  아래 고정 IP 상수는 향후 정적 IP 기능 구현 시 참조용으로 정의되어 있으며,
 *  현재 코드에서는 사용되지 않습니다.
 *  @{
 */
#define ETH_USE_STATIC_IP   0               /**< 1 = 고정 IP, 0 = DHCP (현재 미구현, DHCP 사용 중) */
#define ETH_STATIC_IP       "192.168.1.100" /**< 고정 IPv4 주소 (ETH_USE_STATIC_IP=1 시 사용 예정) */
#define ETH_STATIC_MASK     "255.255.255.0" /**< 서브넷 마스크 (ETH_USE_STATIC_IP=1 시 사용 예정) */
#define ETH_STATIC_GW       "192.168.1.1"   /**< 기본 게이트웨이 (ETH_USE_STATIC_IP=1 시 사용 예정) */
/** @} */

/** @defgroup CFG_PWM RC PWM 출력 (LEDC)
 *  50 Hz, 14비트 해상도, 펄스 범위 1000–2000 µs, 중립 1500 µs.
 *  RC ESC / 서보 인터페이스(예: Cytron MDDS30) 호환.
 *  @{
 */
#define PWM_CH1_GPIO        47              /**< PWM 채널 1 출력 GPIO */
#define PWM_CH2_GPIO        48              /**< PWM 채널 2 출력 GPIO */
#define PWM_LEDC_TIMER      LEDC_TIMER_1   /**< LEDC 타이머 인덱스 */
#define PWM_LEDC_MODE       LEDC_LOW_SPEED_MODE /**< LEDC 속도 모드 */
#define PWM_FREQ_HZ         50              /**< PWM 주파수 (Hz, 표준 RC = 50 Hz) */
#define PWM_RESOLUTION      LEDC_TIMER_14_BIT  /**< 타이머 해상도 (ESP32-S3 저속 최대 = 14비트) */
#define PWM_PERIOD_US       20000           /**< PWM 주기 (마이크로초, 1/50 Hz) */
#define PWM_US_MIN          1000            /**< 최소 펄스 폭 (마이크로초, 완전 역방향) */
#define PWM_US_MAX          2000            /**< 최대 펄스 폭 (마이크로초, 완전 전진) */
#define PWM_US_NEUTRAL      1500            /**< 중립 / 정지 펄스 폭 (마이크로초) */
/** @} */

/** @defgroup CFG_TCP TCP 서버 설정
 *  @{
 */
#define TCP_SERVER_PORT     8080            /**< TCP 수신 포트 */
#define TCP_RX_BUF_SIZE     128             /**< 수신 버퍼 크기 (바이트) */
/** @} */
