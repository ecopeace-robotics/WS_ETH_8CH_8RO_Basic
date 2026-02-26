idf_component_register(
    SRCS
        "main.c"
        "eth_init.c"
        "exio.c"
        "relay_ctrl.c"
        "tcp_server.c"
        "pwm_ctrl.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_netif
        esp_eth
        driver
        lwip
)