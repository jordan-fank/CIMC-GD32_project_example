#ifndef __BTN_APP_H__
#define __BTN_APP_H__

#include "mydefine.h"

void btn_task(void);
void app_ebtn_init(void);


//串口发送标志位---控制串口打印波形
extern uint8_t uart_send_flag;

#endif
