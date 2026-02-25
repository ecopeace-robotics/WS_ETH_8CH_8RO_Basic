
/**
 * @brief TCP 서버 시작 (FreeRTOS 태스크 생성)
 *        IP 획득 이벤트 후 eth_init.c에서 자동 호출됨
 *
 * 프로토콜 (텍스트, newline 종료):
 *   클라이언트 → 서버:
 *     "RELAY <번호> ON\n"   예) "RELAY 1 ON\n"
 *     "RELAY <번호> OFF\n"  예) "RELAY 2 OFF\n"
 *     "STATUS\n"
 *
 *   서버 → 클라이언트:
 *     "OK\n"                명령 성공
 *     "ERROR\n"             잘못된 명령
 *     "R1=ON R2=OFF ...\n"  STATUS 응답
 */
void tcp_server_start(void);