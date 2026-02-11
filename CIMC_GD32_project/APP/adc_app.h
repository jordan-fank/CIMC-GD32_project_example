#ifndef __ADC_APP_H__
#define __ADC_APP_H__

#include "mydefine.h"




void adc_init(void);
void adc_task(void);




void dac_sin_init(void);
void Generate_Sine_Wave(uint16_t* buffer, uint32_t samples, uint16_t amplitude, float phase_shift);



//波形分析标志位
extern uint8_t wave_analysis_flag;
//波形查询类型			0=全部  1=类型  2 = 频率 3=峰峰值
extern uint8_t wave_query_type;		



/*
这里之所以报错因为
每个头文件都会引用这个mydefine.h
但是每次这样引用的时候
都会引用adc_app.h
但是它会编译到extern WaveformInfo wave_data;

*/

//extern WaveformInfo wave_data;



#endif

