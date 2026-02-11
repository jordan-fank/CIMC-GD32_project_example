#ifndef WAVEFORM_ANALYZER_APP_H
#define WAVEFORM_ANALYZER_APP_H



#include "mydefine.h"

// 波形类型定义
typedef enum
{
    ADC_WAVEFORM_DC = 0,
    ADC_WAVEFORM_SINE = 1,
    ADC_WAVEFORM_SQUARE = 2,
    ADC_WAVEFORM_TRIANGLE = 3,
    ADC_WAVEFORM_UNKNOWN = 255
} ADC_WaveformType;

// 谐波分量信息结构体
typedef struct
{
    float frequency;
    float amplitude;
    float phase;
    float relative_amp;
} HarmonicComponent;

// 扩展的波形信息结构体
typedef struct
{
    ADC_WaveformType waveform_type;
    float frequency;
    float vpp;
    float mean;
    float rms;
    float phase;
    HarmonicComponent third_harmonic;
    HarmonicComponent fifth_harmonic;
} WaveformInfo;

// 全局变量声明
extern WaveformInfo wave_data;

/*================================================================================================*/
/* 公共函数声明 (API)                                                                            */
/*================================================================================================*/

/**
 * @brief 初始化FFT模块 (必须在主函数中调用一次)
 */
void My_FFT_Init(void);

/**
 * @brief 获取波形所有信息 (这是你主要调用的函数)
 * @param adc_val_buffer_f ADC采样缓冲区 (长度必须为1024)
 * @return 包含波形所有信息的结构体
 */
WaveformInfo Get_Waveform_Info(uint32_t *adc_val_buffer_f);

/**
 * @brief 将波形类型枚举转换为字符串
 */
const char *GetWaveformTypeString(ADC_WaveformType waveform);

/**
 * @brief 计算波形的峰峰值、均值和有效值
 */
float Get_Waveform_Vpp(uint32_t *adc_val_buffer_f, float *mean, float *rms);

/**
 * @brief 获取FFT结果中特定频率分量的相位
 */
float Get_Component_Phase(const float *fft_complex_output, int component_idx);

/**
 * @brief 计算两个信号之间的相位差
 */
float Calculate_Phase_Difference(float phase1, float phase2);


/**
 * @brief 频率映射函数
 */
float Map_Input_To_FFT_Frequency(float input_frequency);
float Map_FFT_To_Input_Frequency(float fft_frequency);


#endif // WAVEFORM_ANALYZER_APP_H



