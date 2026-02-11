#ifndef __LED_APP_H__
#define __LED_APP_H__

#include "mydefine.h"

extern uint8_t ucLed[6];

void led_task(void);
void breath_led(void);
void wave_led(void);

#endif

