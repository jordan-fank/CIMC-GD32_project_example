#ifndef __4G_WEB_H__
#define __4G_WEB_H__

#include "mydefine.h"

// WebSocket LED控制相关函数声明
void websocket_parse_led_command(uint8_t *buffer, uint16_t length);
void websocket_send_led_response(uint8_t led_num, uint8_t state);
uint8_t websocket_get_led_control_mode(void);  // 获取LED控制模式

// 原有函数声明
void UART6_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void websocket_init(void);
void websocket_task(void);

#endif



