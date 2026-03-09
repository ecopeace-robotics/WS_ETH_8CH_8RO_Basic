#include "tcp_server.h"
#include "board_config.h"
#include "relay_ctrl.h"
#include "pwm_ctrl.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include <string.h>
#include <stdio.h>
#include <errno.h>

static const char *TAG = "TCP_SERVER";

// ─── 명령 파싱 및 처리 ────────────────────────────────────────────────────────

/**
 * @brief 수신된 한 줄 명령을 파싱하고 해당 드라이버를 호출한 뒤 응답 문자열을 작성한다.
 *
 * @param[in]  cmd       null-terminated 명령 문자열 (공백/CR 제거된 상태)
 * @param[out] resp_buf  응답 문자열을 기록할 버퍼
 * @param[in]  buf_size  resp_buf 크기
 * @return 응답 바이트 수
 *
 * @details
 * 명령 파싱 및 디스패치 흐름:
 *
 * \dot
 * digraph HandleCommand {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     input    [label="cmd 문자열"];
 *     resp     [label="resp_buf\n(\"OK\\n\" 또는 상태 또는 오류)"];
 *     p_relay  [label="sscanf:\n\"RELAY N ON|OFF\"", shape=diamond];
 *     p_status [label="strncmp:\n\"STATUS\"",         shape=diamond];
 *     p_pwms   [label="strncmp:\n\"PWM STATUS\"",     shape=diamond];
 *     p_pwm    [label="sscanf:\n\"PWM N UUUU\"",      shape=diamond];
 *
 *     input -> p_relay;
 *     p_relay  -> relay_set    [label="일치"];
 *     p_relay  -> p_status     [label="불일치"];
 *     p_status -> relay_get    [label="일치 (x8)"];
 *     p_status -> p_pwms       [label="불일치"];
 *     p_pwms   -> pwm_get_us   [label="일치 (x2)"];
 *     p_pwms   -> p_pwm        [label="불일치"];
 *     p_pwm    -> pwm_set_us   [label="일치"];
 *     p_pwm    -> "ERROR: unknown" [label="불일치"];
 *
 *     relay_set     -> resp;
 *     relay_get     -> resp;
 *     pwm_get_us    -> resp;
 *     pwm_set_us    -> resp;
 *     "ERROR: unknown" -> resp;
 * }
 * \enddot
 */
static int handle_command(const char *cmd, char *resp_buf, int buf_size)
{
    int relay_num;
    char state_str[8] = {0};

    // "RELAY <1~8> ON|OFF"
    if (sscanf(cmd, "RELAY %d %7s", &relay_num, state_str) == 2) {
        if (relay_num < 1 || relay_num > 8) {
            return snprintf(resp_buf, buf_size, "ERROR: relay number must be 1~8\n");
        }

        relay_id_t id = (relay_id_t)(relay_num - 1);
        bool on;

        if (strcmp(state_str, "ON") == 0) {
            on = true;
        } else if (strcmp(state_str, "OFF") == 0) {
            on = false;
        } else {
            return snprintf(resp_buf, buf_size, "ERROR: state must be ON or OFF\n");
        }

        esp_err_t err = relay_set(id, on);
        if (err != ESP_OK) {
            return snprintf(resp_buf, buf_size, "ERROR: relay_set failed\n");
        }
        return snprintf(resp_buf, buf_size, "OK\n");
    }

    // "STATUS"
    if (strncmp(cmd, "STATUS", 6) == 0) {
        int len = 0;
        for (int i = 0; i < RELAY_MAX; i++) {
            len += snprintf(resp_buf + len, buf_size - len,
                            "R%d=%s ", i + 1,
                            relay_get((relay_id_t)i) ? "ON" : "OFF");
        }
        // 마지막 공백을 개행으로 교체
        if (len > 0 && resp_buf[len - 1] == ' ') {
            resp_buf[len - 1] = '\n';
        }
        return len;
    }

    // "PWM STATUS"
    if (strncmp(cmd, "PWM STATUS", 10) == 0) {
        return snprintf(resp_buf, buf_size, "PWM1=%d PWM2=%d\n",
                        pwm_get_us(1), pwm_get_us(2));
    }

    // "PWM <1|2> <1000-2000>"
    {
        int ch;
        int us;
        if (sscanf(cmd, "PWM %d %d", &ch, &us) == 2) {
            if (ch < 1 || ch > 2) {
                return snprintf(resp_buf, buf_size, "ERROR: PWM channel must be 1 or 2\n");
            }
            if (us < PWM_US_MIN || us > PWM_US_MAX) {
                return snprintf(resp_buf, buf_size,
                                "ERROR: pulse width must be %d~%d µs\n",
                                PWM_US_MIN, PWM_US_MAX);
            }
            esp_err_t err = pwm_set_us((uint8_t)ch, (uint16_t)us);
            if (err != ESP_OK) {
                return snprintf(resp_buf, buf_size, "ERROR: pwm_set_us failed\n");
            }
            return snprintf(resp_buf, buf_size, "OK\n");
        }
    }

    return snprintf(resp_buf, buf_size, "ERROR: unknown command\n");
}

// ─── 클라이언트 처리 태스크 ───────────────────────────────────────────────────

/**
 * @brief 개별 TCP 클라이언트 처리 태스크.
 *        한 클라이언트당 하나의 태스크가 생성된다.
 *
 * @param[in] arg  클라이언트 소켓 FD (int, intptr_t 캐스팅)
 *
 * @details
 * 수신 → 줄 파싱 → 명령 처리 루프:
 *
 * \dot
 * digraph ClientTask {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     start  [label="클라이언트 연결\n환영 메시지 전송"];
 *     recv   [label="recv() -> rx_buf"];
 *     chk    [label="수신 <= 0?", shape=diamond];
 *     find   [label="strchr(rx_buf, '\\n')", shape=diamond];
 *     handle [label="handle_command(line)\nsend(resp)"];
 *     shift  [label="memmove: 잔여\n바이트를 buf[0]으로"];
 *     ovf    [label="버퍼 가득참?\nrx_len 초기화", shape=diamond];
 *     done   [label="close(sock)\nvTaskDelete"];
 *
 *     start -> recv;
 *     recv  -> chk;
 *     chk   -> done  [label="yes"];
 *     chk   -> find  [label="no"];
 *     find  -> handle [label="'\\n' 발견"];
 *     handle -> find;
 *     find  -> shift  [label="더 이상 줄 없음"];
 *     shift -> ovf;
 *     ovf   -> recv;
 * }
 * \enddot
 */
static void client_task(void *arg)
{
    int client_sock = (int)(intptr_t)arg;
    char rx_buf[TCP_RX_BUF_SIZE] = {0};
    char resp_buf[256] = {0};
    int rx_len = 0;

    ESP_LOGI(TAG, "Client connected, sock=%d", client_sock);

    // 환영 메시지
    const char *welcome = "ESP32 Relay Controller ready. Commands: RELAY <1-8> ON|OFF, STATUS\n";
    send(client_sock, welcome, strlen(welcome), 0);

    while (1) {
        // 남은 버퍼에 수신
        int received = recv(client_sock,
                            rx_buf + rx_len,
                            sizeof(rx_buf) - rx_len - 1,
                            0);
        if (received <= 0) {
            if (received < 0) {
                ESP_LOGW(TAG, "recv error: errno=%d", errno);
            } else {
                ESP_LOGI(TAG, "Client disconnected, sock=%d", client_sock);
            }
            break;
        }

        rx_len += received;
        rx_buf[rx_len] = '\0';

        // 줄 단위로 명령 처리 (\n 기준)
        char *line_start = rx_buf;
        char *newline;
        while ((newline = strchr(line_start, '\n')) != NULL) {
            *newline = '\0';

            // 앞뒤 공백/CR 제거
            while (*line_start == ' ' || *line_start == '\r') line_start++;
            char *end = newline - 1;
            while (end > line_start && (*end == ' ' || *end == '\r')) {
                *end-- = '\0';
            }

            if (strlen(line_start) > 0) {
                ESP_LOGI(TAG, "CMD: \"%s\"", line_start);
                int resp_len = handle_command(line_start, resp_buf, sizeof(resp_buf));
                send(client_sock, resp_buf, resp_len, 0);
            }

            line_start = newline + 1;
        }

        // 처리되지 않은 잔여 데이터를 버퍼 앞으로 이동
        int remaining = rx_len - (line_start - rx_buf);
        if (remaining > 0) {
            memmove(rx_buf, line_start, remaining);
        }
        rx_len = remaining;
        rx_buf[rx_len] = '\0';

        // 버퍼 오버플로우 방지
        if (rx_len >= (int)(sizeof(rx_buf) - 1)) {
            ESP_LOGW(TAG, "Buffer overflow, clearing");
            rx_len = 0;
        }
    }

    close(client_sock);
    ESP_LOGI(TAG, "Client task done, sock=%d", client_sock);
    vTaskDelete(NULL);
}

// ─── 서버 리슨 태스크 ─────────────────────────────────────────────────────────

/**
 * @brief TCP 서버 리슨 태스크. accept() 루프로 클라이언트 연결을 수락하고
 *        연결마다 client_task를 생성한다.
 *
 * @param[in] arg  미사용 (NULL)
 *
 * @details
 * 서버 소켓 설정 및 accept 루프:
 *
 * \dot
 * digraph TcpServerTask {
 *     node [shape=box, fontname="Helvetica", fontsize=10];
 *     edge [fontname="Helvetica", fontsize=9];
 *     client_task_node [label="client_task\n(FreeRTOS task)", style=filled, fillcolor=lightyellow];
 *
 *     socket  [label="socket(AF_INET)"];
 *     bind    [label="bind(port 8080)"];
 *     listen  [label="listen(backlog=4)"];
 *     accept  [label="accept()"];
 *     chk     [label="client_sock < 0?", shape=diamond];
 *     create  [label="xTaskCreate\nclient_task"];
 *
 *     socket -> bind -> listen -> accept;
 *     accept -> chk;
 *     chk -> accept [label="yes (continue)"];
 *     chk -> create [label="no"];
 *     create -> client_task_node;
 *     create -> accept [label="루프"];
 * }
 * \enddot
 */
static void tcp_server_task(void *arg)
{
    struct sockaddr_in server_addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port        = htons(TCP_SERVER_PORT),
    };

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "socket() failed: errno=%d", errno);
        vTaskDelete(NULL);
        return;
    }

    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        ESP_LOGE(TAG, "bind() failed: errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_sock, 4) != 0) {
        ESP_LOGE(TAG, "listen() failed: errno=%d", errno);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "TCP server listening on port %d", TCP_SERVER_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_sock = accept(listen_sock,
                                 (struct sockaddr *)&client_addr,
                                 &addr_len);
        if (client_sock < 0) {
            ESP_LOGW(TAG, "accept() failed: errno=%d", errno);
            continue;
        }

        char addr_str[16];
        inet_ntoa_r(client_addr.sin_addr, addr_str, sizeof(addr_str));
        ESP_LOGI(TAG, "New connection from %s:%d",
                 addr_str, ntohs(client_addr.sin_port));

        // 클라이언트마다 별도 태스크 생성
        xTaskCreate(client_task, "tcp_client",
                    4096, (void *)(intptr_t)client_sock,
                    5, NULL);
    }

    close(listen_sock);
    vTaskDelete(NULL);
}

// ─── 공개 함수 ────────────────────────────────────────────────────────────────

/**
 * @brief TCP 서버 태스크를 생성한다.
 *
 * @details
 * tcp_server_task를 FreeRTOS 태스크로 생성한다.
 * eth_init.c의 IP 획득 이벤트 핸들러에서 최초 1회 호출된다.
 * @note 파라미터/반환값 명세는 tcp_server.h 참조.
 */
void tcp_server_start(void)
{
    xTaskCreate(tcp_server_task, "tcp_server",
                4096, NULL,
                5, NULL);
    ESP_LOGI(TAG, "TCP server task created");
}
