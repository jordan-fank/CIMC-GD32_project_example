#ifndef __UART_APP_H__
#define __UART_APP_H__

#include "mydefine.h"

//重定向串口打印
int my_printf(UART_HandleTypeDef *huart, const char *format, ...);


void uart_init(void);
void uart_task(void);

void UART1_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);




#endif



