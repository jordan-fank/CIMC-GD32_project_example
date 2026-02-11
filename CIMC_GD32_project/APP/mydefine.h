/*==============================各模块头文件===================*/
#ifndef __MYDEFINE_APP_H__
#define __MYDEFINE_APP_H__

//--------------------------外设底层驱动--------------------//
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "sdio.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"


//--------------------------C语言标准库--------------------//
#include "stdarg.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
#include "math.h"
#include "float.h"
#include "stdlib.h"


//---------------------------组件库-----------------------//
#include "btn_app.h"				//按键库
#include "ebtn.h"					//按键库
#include "oled.h"					//OLED
#include "ringbuffer.h"				//环形缓冲区
#include "arm_math.h"				//启用这个DSP高级运算
#include "waveform_analyzer_app.h"	//傅里叶变换，波形分析

#include "u8g2.h"

#include "WouoUI.h"      			// WouoUI 核心框架
#include "WouoUI_user.h" 			// 用户自定义的菜单结构和回调函数 (通常需要用户创建或修改)
			
#include "gd25qxx.h"	 			//flash底层驱动的头文件
			
#include "lfs.h"		 			//LittleFs的驱动库头文件
#include "lfs_port.h"	 			//LittleFs的驱动库头文件






//---------------------------外设应用---------------------//
#include "scheduler.h"

#include "led_app.h"

#include "key_app.h"

#include "uart_app.h"

#include "adc_app.h"
#include "dac_app.h"

#include "oled_app.h"
#include "u8g2_port.h"
#include "wououi_app.h"

#include "flash_app.h"
#include "shell_app.h"

#include "sd_fatfs.h"

#include "4g_web.h"














//----------------------需要引用的外部变量---------------------//




extern lfs_t lfs;
extern struct lfs_config cfg;

#endif



