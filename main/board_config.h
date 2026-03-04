/**
 * @file board_config.h
 * @brief Hardware pin assignments and compile-time configuration constants
 *        for the Waveshare ESP32-S3-ETH-8DI-8RO board.
 *
 * Edit this file to adapt the firmware to different GPIO assignments,
 * network settings, or PWM parameters without touching driver code.
 */

#pragma once

/** @defgroup CFG_ETH Ethernet (W5500 SPI) pin and clock settings
 *  @{
 */
#define ETH_SPI_HOST        SPI2_HOST   /**< SPI host peripheral used for W5500 */
#define ETH_SPI_CLOCK_MHZ   20          /**< SPI clock speed in MHz */

#define ETH_MOSI_GPIO       13          /**< SPI MOSI GPIO */
#define ETH_MISO_GPIO       14          /**< SPI MISO GPIO */
#define ETH_SCLK_GPIO       15          /**< SPI SCLK GPIO */
#define ETH_CS_GPIO         16          /**< W5500 chip-select GPIO */
#define ETH_INT_GPIO        12          /**< W5500 interrupt GPIO */
#define ETH_RST_GPIO        39          /**< W5500 hardware reset GPIO */
/** @} */

/** @defgroup CFG_EXIO TCA9554PWR I2C IO Expander (relay control)
 *  A0=A1=A2=GND → default I2C address 0x20
 *  @{
 */
#define EXIO_SCL_GPIO       41          /**< I2C SCL GPIO */
#define EXIO_SDA_GPIO       42          /**< I2C SDA GPIO */
#define EXIO_I2C_ADDR       0x20        /**< 7-bit I2C address (A0=A1=A2=GND) */
#define EXIO_I2C_FREQ_HZ    100000      /**< I2C clock frequency in Hz */

/** @defgroup CFG_TCA9554_REGS TCA9554 internal register addresses
 *  @{
 */
#define TCA9554_REG_INPUT       0x00    /**< Input port register (read-only) */
#define TCA9554_REG_OUTPUT      0x01    /**< Output port register */
#define TCA9554_REG_POLARITY    0x02    /**< Polarity inversion register */
#define TCA9554_REG_CONFIG      0x03    /**< Configuration register: 0=output, 1=input */
/** @} */
/** @} */

/** @defgroup CFG_NET Network settings
 *  Set #ETH_USE_STATIC_IP to 0 to use DHCP instead of a fixed IP.
 *  @{
 */
#define ETH_USE_STATIC_IP   1               /**< 1 = static IP, 0 = DHCP */
#define ETH_STATIC_IP       "192.168.1.100" /**< Static IPv4 address */
#define ETH_STATIC_MASK     "255.255.255.0" /**< Subnet mask */
#define ETH_STATIC_GW       "192.168.1.1"   /**< Default gateway */
/** @} */

/** @defgroup CFG_PWM RC PWM output (LEDC)
 *  50 Hz, 14-bit resolution, pulse range 1000–2000 µs, neutral 1500 µs.
 *  Compatible with RC ESC / servo interfaces (e.g. Cytron MDDS30).
 *  @{
 */
#define PWM_CH1_GPIO        47              /**< PWM channel 1 output GPIO */
#define PWM_CH2_GPIO        48              /**< PWM channel 2 output GPIO */
#define PWM_LEDC_TIMER      LEDC_TIMER_1   /**< LEDC timer index */
#define PWM_LEDC_MODE       LEDC_LOW_SPEED_MODE /**< LEDC speed mode */
#define PWM_FREQ_HZ         50              /**< PWM frequency in Hz (standard RC = 50 Hz) */
#define PWM_RESOLUTION      LEDC_TIMER_14_BIT  /**< Timer resolution (ESP32-S3 low-speed max = 14-bit) */
#define PWM_PERIOD_US       20000           /**< PWM period in microseconds (1/50 Hz) */
#define PWM_US_MIN          1000            /**< Minimum pulse width in microseconds (full reverse) */
#define PWM_US_MAX          2000            /**< Maximum pulse width in microseconds (full forward) */
#define PWM_US_NEUTRAL      1500            /**< Neutral / stop pulse width in microseconds */
/** @} */

/** @defgroup CFG_TCP TCP server settings
 *  @{
 */
#define TCP_SERVER_PORT     8080            /**< TCP listening port */
#define TCP_RX_BUF_SIZE     128             /**< Receive buffer size in bytes */
/** @} */