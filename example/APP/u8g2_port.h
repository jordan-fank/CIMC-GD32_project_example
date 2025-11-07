#ifndef __URG2_PORT_H__
#define __URG2_PORT_H__

#include "mydefine.h"

extern u8g2_t u8g2;


uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);


void u8g2_init(void);	//u8g2初始化函数
void u8g2_task(void);	//u8g2任务处理函数

void OLED_SendBuff(uint8_t buff[4][128]);

#endif

