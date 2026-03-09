/**
 * @file tcp_server.h
 * @brief 네트워크 클라이언트로부터 텍스트 명령을 수신하고
 *        릴레이 및 PWM 제어 모듈로 디스패치하는 TCP 서버 태스크.
 *
 * 서버는 TCP_SERVER_PORT (기본값 8080)에서 수신 대기한다.
 * IP 주소 획득 시 eth_init()에 의해 자동으로 시작된다.
 */

#pragma once

/**
 * @defgroup TCPServer TCP 명령 서버
 * @brief 텍스트 기반 명령 인터프리터 (릴레이, PWM 제어)
 * @ingroup Application
 * @see RelayControl PWMControl
 * @{
 */

/**
 * @brief TCP 서버 시작 (FreeRTOS 태스크 생성)
 *        IP 획득 이벤트 후 eth_init.c에서 자동 호출됨
 *
 * @details
 * 프로토콜 (텍스트, newline 종료):
 *
 * @code
 * // 클라이언트 → 서버 (명령):
 * "RELAY 1 ON\n"          // 릴레이 1 켜기
 * "RELAY 2 OFF\n"         // 릴레이 2 끄기
 * "STATUS\n"              // 전체 릴레이 상태 조회
 * "PWM 1 1500\n"          // PWM 채널 1을 1500 µs로 설정
 * "PWM STATUS\n"          // PWM 채널 상태 조회
 *
 * // 서버 → 클라이언트 (응답):
 * "OK\n"                  // 명령 성공
 * "ERROR: ...\n"          // 명령 오류 (상세 메시지)
 * "R1=ON R2=OFF R3=OFF ...\n"   // STATUS 응답 (최대 8개 릴레이)
 * "PWM1=1500 PWM2=1500\n"       // PWM STATUS 응답
 * @endcode
 */
void tcp_server_start(void);

/** @} */  // TCPServer 그룹 끝
