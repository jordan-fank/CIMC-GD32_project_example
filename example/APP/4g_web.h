#ifndef __4G_WEB_H__
#define __4G_WEB_H__

#include "mydefine.h"

void UART6_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void websocket_init(void);

void websocket_task(void);

#endif



