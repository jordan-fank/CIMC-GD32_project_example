/********************************************************************************
 * @file    waveform_analyzer_app.c
 * @author  (Original Author), Gemini (Modifier)
 * @brief   波形分析功能实现文件。
 *
 * @note    【最终完整修复版 - 20251019】
 * 1. [错误修复] 修复了Get_Component_Phase函数定义与声明中const不匹配的问题。
 * 2. [警告修复] 使用memset替代{0}来初始化WaveformInfo结构体，消除类型混合警告。
 * 3. [崩溃修复] 保留了将Analyze_Harmonics函数中巨大数组声明为static的核心修复。
 * 4. [完整性] 本文件为完整版本，可直接替换旧文件并编译。
 ********************************************************************************/

#include "waveform_analyzer_app.h"



#define FFT_LENGTH 1024						//
arm_cfft_radix4_instance_f32 scfft;
float FFT_InputBuf[FFT_LENGTH * 2];
float FFT_OutputBuf[FFT_LENGTH];

// --- 函数声明 (保持原始结构) ---
float Map_Input_To_FFT_Frequency(float input_frequency);
float Map_FFT_To_Input_Frequency(float fft_frequency);
void My_FFT_Init(void);
float Get_Waveform_Vpp(uint32_t *adc_val_buffer_f, float *mean, float *rms);
void Perform_FFT(uint32_t *adc_val_buffer_f);
float Get_Component_Phase(const float *fft_buffer, int component_idx); // 【错误修复】添加const
float Get_Waveform_Phase(uint32_t *adc_val_buffer_f, float frequency);
float Get_Waveform_Phase_ZeroCrossing(uint32_t *adc_val_buffer_f, float frequency);
float Calculate_Phase_Difference(float phase1, float phase2);
float Get_Waveform_Frequency(uint32_t *adc_val_buffer_f);
void Analyze_Harmonics(uint32_t *adc_val_buffer_f, WaveformInfo *waveform_info);
ADC_WaveformType Analyze_Frequency_And_Type(uint32_t *adc_val_buffer_f, float *signal_frequency);
ADC_WaveformType Get_Waveform_Type(uint32_t *adc_val_buffer_f);
const char *GetWaveformTypeString(ADC_WaveformType waveform);
float Get_Phase_Difference(uint32_t *adc_val_buffer1, uint32_t *adc_val_buffer2, float frequency);
WaveformInfo Get_Waveform_Info(uint32_t *adc_val_buffer_f);

// --- 函数实现 ---

float Map_Input_To_FFT_Frequency(float input_frequency)
{
    if (input_frequency <= 2600.0f) return input_frequency;
    else if (input_frequency <= 6100.0f) return input_frequency * 2.0f;
    else if (input_frequency <= 8100.0f) return input_frequency * 3.0f;
    else if (input_frequency <= 11100.0f) return input_frequency * 4.0f;
    else if (input_frequency <= 14100.0f) return input_frequency * 5.0f;
    else if (input_frequency <= 17100.0f) return input_frequency * 6.0f;
    else if (input_frequency <= 19600.0f) return input_frequency * 7.0f;
    else if (input_frequency <= 21600.0f) return input_frequency * 8.0f;
    else if (input_frequency <= 25100.0f) return input_frequency * 9.0f;
    else if (input_frequency <= 26600.0f) return input_frequency * 10.0f;
    else if (input_frequency <= 29600.0f) return input_frequency * 11.0f;
    else if (input_frequency <= 32100.0f) return input_frequency * 12.0f;
    else return input_frequency * 13.0f;
}

float Map_FFT_To_Input_Frequency(float fft_frequency)
{
    const float breaks[] = {2600.0f, 6100.0f, 8100.0f, 11100.0f, 14100.0f, 17100.0f, 19600.0f, 21600.0f, 25100.0f, 26600.0f, 29600.0f, 32100.0f};
    const float dividers[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f};
    const int num_breaks = sizeof(breaks) / sizeof(breaks[0]);

    float last_break_fft = breaks[num_breaks - 1] * dividers[num_breaks];
    if (fft_frequency > last_break_fft)
    {
        return fft_frequency / dividers[num_breaks];
    }
    for (int i = 0; i < num_breaks; i++)
    {
        float break_fft = breaks[i] * dividers[i];
        if (fft_frequency <= break_fft)
        {
            return fft_frequency / dividers[i];
        }
        float next_break_fft = (i < num_breaks - 1) ? breaks[i + 1] * dividers[i + 1] : FLT_MAX;
        if (fft_frequency < next_break_fft)
        {
            return fft_frequency / dividers[i + 1];
        }
    }
    return fft_frequency / dividers[num_breaks];
}

void My_FFT_Init(void)
{
    arm_cfft_radix4_init_f32(&scfft, FFT_LENGTH, 0, 1);
}

float Get_Waveform_Vpp(uint32_t *adc_val_buffer_f, float *mean, float *rms)
{
    float min_val = 3.3f;
    float max_val = 0.0f;
    float sum = 0.0f;
    float sum_squares = 0.0f;
    float voltage;
    for (int i = 0; i < FFT_LENGTH; i++)
    {
        voltage = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        if (voltage > max_val) max_val = voltage;
        if (voltage < min_val) min_val = voltage;
        sum += voltage;
        sum_squares += voltage * voltage;
    }
    *mean = sum / (float)FFT_LENGTH;
    *rms = sqrtf(sum_squares / (float)FFT_LENGTH);
    return max_val - min_val;
}

void Perform_FFT(uint32_t *adc_val_buffer_f)
{
    for (int i = 0; i < FFT_LENGTH; i++)
    {
        FFT_InputBuf[2 * i] = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        FFT_InputBuf[2 * i + 1] = 0;
    }
    arm_cfft_radix4_f32(&scfft, FFT_InputBuf);
    arm_cmplx_mag_f32(FFT_InputBuf, FFT_OutputBuf, FFT_LENGTH);
}

// 【【【 核心错误修复 】】】 添加 const 关键字，与 .h 文件声明保持一致
float Get_Component_Phase(const float *fft_buffer, int component_idx)
{
    float real_part = fft_buffer[2 * component_idx];
    float imag_part = fft_buffer[2 * component_idx + 1];
    return atan2f(imag_part, real_part);
}

float Get_Waveform_Phase(uint32_t *adc_val_buffer_f, float frequency)
{
    if (frequency <= 0.0f) return 0.0f;

    for (int i = 0; i < FFT_LENGTH; i++)
    {
        FFT_InputBuf[2 * i] = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        FFT_InputBuf[2 * i + 1] = 0;
    }
    arm_cfft_radix4_f32(&scfft, FFT_InputBuf);

    float fft_frequency = Map_Input_To_FFT_Frequency(frequency);
    float sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    float sampling_frequency = 1000000.0f / sampling_interval_us;
    int fundamental_idx = (int)(fft_frequency * FFT_LENGTH / sampling_frequency + 0.5f);

    if (fundamental_idx <= 0 || fundamental_idx >= FFT_LENGTH / 2) return 0.0f;
    return Get_Component_Phase(FFT_InputBuf, fundamental_idx);
}

float Get_Waveform_Phase_ZeroCrossing(uint32_t *adc_val_buffer_f, float frequency)
{
    if (frequency <= 0.0f) return 0.0f;

    float sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    float sampling_frequency = 1000000.0f / sampling_interval_us;
    float mean = 0.0f;
    for (int i = 0; i < FFT_LENGTH; i++)
    {
        mean += (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
    }
    mean /= (float)FFT_LENGTH;

    int zero_crossing_idx = -1;
    for (int i = 1; i < FFT_LENGTH; i++)
    {
        float current_val = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f - mean;
        float prev_val = (float)adc_val_buffer_f[i - 1] / 4096.0f * 3.3f - mean;
        if (prev_val < 0.0f && current_val >= 0.0f)
        {
            zero_crossing_idx = i;
            break;
        }
    }

    if (zero_crossing_idx < 0) return 0.0f;

    float prev_val = (float)adc_val_buffer_f[zero_crossing_idx - 1] / 4096.0f * 3.3f - mean;
    float current_val = (float)adc_val_buffer_f[zero_crossing_idx] / 4096.0f * 3.3f - mean;
    float fraction = -prev_val / (current_val - prev_val);
    float exact_crossing = (float)(zero_crossing_idx - 1) + fraction;
    float fft_frequency = Map_Input_To_FFT_Frequency(frequency);
    float samples_per_period = sampling_frequency / fft_frequency;
    float phase = 2.0f * PI * exact_crossing / samples_per_period;

    while (phase < 0.0f) phase += 2.0f * PI;
    while (phase >= 2.0f * PI) phase -= 2.0f * PI;

    return phase;
}

float Calculate_Phase_Difference(float phase1, float phase2)
{
    float phase_diff = phase1 - phase2;
    while (phase_diff > PI) phase_diff -= 2.0f * PI;
    while (phase_diff <= -PI) phase_diff += 2.0f * PI;
    return phase_diff;
}

float Get_Waveform_Frequency(uint32_t *adc_val_buffer_f)
{
    float sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    float sampling_frequency = 1000000.0f / sampling_interval_us;
    Perform_FFT(adc_val_buffer_f);

    float max_harmonic = 0;
    int max_harmonic_idx = 0;
    for (int i = 1; i < FFT_LENGTH / 2; i++)
    {
        if (FFT_OutputBuf[i] > max_harmonic)
        {
            max_harmonic = FFT_OutputBuf[i];
            max_harmonic_idx = i;
        }
    }
    float fft_frequency = (float)max_harmonic_idx * sampling_frequency / (float)FFT_LENGTH;
    if (FFT_OutputBuf[0] > max_harmonic * 5.0f)
    {
        return 0.0f;
    }
    return Map_FFT_To_Input_Frequency(fft_frequency);
}

void Analyze_Harmonics(uint32_t *adc_val_buffer_f, WaveformInfo *waveform_info)
{
    float sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    float sampling_frequency = 1000000.0f / sampling_interval_us;

    if (waveform_info->frequency <= 0.0f)
    {
        waveform_info->third_harmonic.frequency = 0.0f;
        waveform_info->third_harmonic.amplitude = 0.0f;
        waveform_info->third_harmonic.phase = 0.0f;
        waveform_info->third_harmonic.relative_amp = 0.0f;
        waveform_info->fifth_harmonic.frequency = 0.0f;
        waveform_info->fifth_harmonic.amplitude = 0.0f;
        waveform_info->fifth_harmonic.phase = 0.0f;
        waveform_info->fifth_harmonic.relative_amp = 0.0f;
        return;
    }

    for (int i = 0; i < FFT_LENGTH; i++)
    {
        FFT_InputBuf[2 * i] = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        FFT_InputBuf[2 * i + 1] = 0;
    }
    arm_cfft_radix4_f32(&scfft, FFT_InputBuf);

    // 【崩溃修复】保留此关键修改：将4KB数组声明为static
    static float magnitude_spectrum[FFT_LENGTH];
    arm_cmplx_mag_f32(FFT_InputBuf, magnitude_spectrum, FFT_LENGTH);

    float fft_frequency = Map_Input_To_FFT_Frequency(waveform_info->frequency);
    int fundamental_idx = (int)(fft_frequency * FFT_LENGTH / sampling_frequency + 0.5f);
    float fundamental_amp = magnitude_spectrum[fundamental_idx];
    float fundamental_phase = Get_Component_Phase(FFT_InputBuf, fundamental_idx);

    int expected_third_harmonic_idx = 3 * fundamental_idx;
    int third_search_start = expected_third_harmonic_idx - fundamental_idx / 4;
    int third_search_end = expected_third_harmonic_idx + fundamental_idx / 4;
    if (third_search_start < fundamental_idx) third_search_start = fundamental_idx + 1;
    if (third_search_end >= FFT_LENGTH / 2) third_search_end = FFT_LENGTH / 2 - 1;

    float third_harmonic_amp = 0.0f;
    int third_harmonic_idx = 0;
    for (int i = third_search_start; i <= third_search_end; i++)
    {
        if (magnitude_spectrum[i] > third_harmonic_amp)
        {
            third_harmonic_amp = magnitude_spectrum[i];
            third_harmonic_idx = i;
        }
    }

    int expected_fifth_harmonic_idx = 5 * fundamental_idx;
    int fifth_search_start = expected_fifth_harmonic_idx - fundamental_idx / 4;
    int fifth_search_end = expected_fifth_harmonic_idx + fundamental_idx / 4;
    if (fifth_search_start < third_harmonic_idx + 1) fifth_search_start = third_harmonic_idx + 1;
    if (fifth_search_end >= FFT_LENGTH / 2) fifth_search_end = FFT_LENGTH / 2 - 1;

    float fifth_harmonic_amp = 0.0f;
    int fifth_harmonic_idx = 0;
    for (int i = fifth_search_start; i <= fifth_search_end; i++)
    {
        if (magnitude_spectrum[i] > fifth_harmonic_amp)
        {
            fifth_harmonic_amp = magnitude_spectrum[i];
            fifth_harmonic_idx = i;
        }
    }

    if (third_harmonic_amp < fundamental_amp * 0.05f)
    {
        third_harmonic_amp = 0.0f;
        third_harmonic_idx = 0;
    }
    if (fifth_harmonic_amp < fundamental_amp * 0.05f)
    {
        fifth_harmonic_amp = 0.0f;
        fifth_harmonic_idx = 0;
    }

    float third_harmonic_phase = (third_harmonic_idx > 0) ? Get_Component_Phase(FFT_InputBuf, third_harmonic_idx) : 0.0f;
    float fifth_harmonic_phase = (fifth_harmonic_idx > 0) ? Get_Component_Phase(FFT_InputBuf, fifth_harmonic_idx) : 0.0f;
    float third_harmonic_fft_freq = (third_harmonic_idx > 0) ? (float)third_harmonic_idx * sampling_frequency / (float)FFT_LENGTH : 0.0f;
    float fifth_harmonic_fft_freq = (fifth_harmonic_idx > 0) ? (float)fifth_harmonic_idx * sampling_frequency / (float)FFT_LENGTH : 0.0f;
    float third_harmonic_freq = (third_harmonic_idx > 0) ? Map_FFT_To_Input_Frequency(third_harmonic_fft_freq) : 0.0f;
    float fifth_harmonic_freq = (fifth_harmonic_idx > 0) ? Map_FFT_To_Input_Frequency(fifth_harmonic_fft_freq) : 0.0f;

    waveform_info->third_harmonic.frequency = third_harmonic_freq;
    waveform_info->third_harmonic.amplitude = third_harmonic_amp;
    waveform_info->third_harmonic.phase = third_harmonic_phase;
    waveform_info->third_harmonic.relative_amp = (fundamental_amp > 0.0f) ? third_harmonic_amp / fundamental_amp : 0.0f;

    waveform_info->fifth_harmonic.frequency = fifth_harmonic_freq;
    waveform_info->fifth_harmonic.amplitude = fifth_harmonic_amp;
    waveform_info->fifth_harmonic.phase = fifth_harmonic_phase;
    waveform_info->fifth_harmonic.relative_amp = (fundamental_amp > 0.0f) ? fifth_harmonic_amp / fundamental_amp : 0.0f;

    if (third_harmonic_idx > 0)
    {
        float third_phase_diff = Calculate_Phase_Difference(third_harmonic_phase, fundamental_phase);
        waveform_info->third_harmonic.phase = third_phase_diff;
    }
    if (fifth_harmonic_idx > 0)
    {
        float fifth_phase_diff = Calculate_Phase_Difference(fifth_harmonic_phase, fundamental_phase);
        waveform_info->fifth_harmonic.phase = fifth_phase_diff;
    }
}

ADC_WaveformType Analyze_Frequency_And_Type(uint32_t *adc_val_buffer_f, float *signal_frequency)
{
    float sampling_interval_us = dac_app_get_adc_sampling_interval_us();
    float sampling_frequency = 1000000.0f / sampling_interval_us;

    for (int i = 0; i < FFT_LENGTH; i++)
    {
        FFT_InputBuf[2 * i] = (float)adc_val_buffer_f[i] / 4096.0f * 3.3f;
        FFT_InputBuf[2 * i + 1] = 0;
    }
    arm_cfft_radix4_f32(&scfft, FFT_InputBuf);
    arm_cmplx_mag_f32(FFT_InputBuf, FFT_OutputBuf, FFT_LENGTH);

    float dc_component = FFT_OutputBuf[0];
    float fundamental_amp = 0.0f;
    int fundamental_idx = 0;
    for (int i = 1; i < FFT_LENGTH / 2; i++)
    {
        if (FFT_OutputBuf[i] > fundamental_amp)
        {
            fundamental_amp = FFT_OutputBuf[i];
            fundamental_idx = i;
        }
    }

    float fft_frequency = (float)fundamental_idx * sampling_frequency / (float)FFT_LENGTH;
    *signal_frequency = Map_FFT_To_Input_Frequency(fft_frequency);

    if (dc_component > fundamental_amp * 5.0f)
    {
        *signal_frequency = 0.0f;
        return ADC_WAVEFORM_DC;
    }

    if (fundamental_amp < 5.0f)
    {
        return ADC_WAVEFORM_UNKNOWN;
    }

    float third_harmonic_amp = 0.0f;
    int start_third = 2 * fundamental_idx;
    int end_third = 4 * fundamental_idx;
    if (end_third > FFT_LENGTH / 2) end_third = FFT_LENGTH / 2;
    for (int i = start_third; i < end_third; i++)
    {
        if (FFT_OutputBuf[i] > third_harmonic_amp)
        {
            third_harmonic_amp = FFT_OutputBuf[i];
        }
    }

    float fifth_harmonic_amp = 0.0f;
    int start_fifth = 4 * fundamental_idx;
    int end_fifth = 6 * fundamental_idx;
    if (end_fifth > FFT_LENGTH / 2) end_fifth = FFT_LENGTH / 2;
    for (int i = start_fifth; i < end_fifth; i++)
    {
        if (FFT_OutputBuf[i] > fifth_harmonic_amp)
        {
            fifth_harmonic_amp = FFT_OutputBuf[i];
        }
    }

    if (third_harmonic_amp < fundamental_amp * 0.05f) third_harmonic_amp = 0.0f;
    if (fifth_harmonic_amp < fundamental_amp * 0.05f) fifth_harmonic_amp = 0.0f;

    float third_ratio = (fundamental_amp > 0.0f) ? (third_harmonic_amp / fundamental_amp) : 0;
    float fifth_ratio = (fundamental_amp > 0.0f) ? (fifth_harmonic_amp / fundamental_amp) : 0;

    const float TRIANGLE_THIRD_END = 1.0f / 15.0f;
    const float SQUARE_THIRD_END = 1.0f / 5.0f;

    if (third_ratio < 0.05f && fifth_ratio < 0.05f) return ADC_WAVEFORM_SINE;
    if (third_ratio > SQUARE_THIRD_END) return ADC_WAVEFORM_SQUARE;
    if (third_ratio > TRIANGLE_THIRD_END) return ADC_WAVEFORM_TRIANGLE;
    
    return ADC_WAVEFORM_UNKNOWN;
}

ADC_WaveformType Get_Waveform_Type(uint32_t *adc_val_buffer_f)
{
    float frequency;
    return Analyze_Frequency_And_Type(adc_val_buffer_f, &frequency);
}

const char *GetWaveformTypeString(ADC_WaveformType waveform)
{
    switch (waveform)
    {
        case ADC_WAVEFORM_DC:       return "直流";
        case ADC_WAVEFORM_SINE:     return "正弦波";
        case ADC_WAVEFORM_TRIANGLE: return "三角波";
        case ADC_WAVEFORM_SQUARE:   return "方波";
        case ADC_WAVEFORM_UNKNOWN:  return "未知波形";
        default:                    return "无效波形";
    }
}

float Get_Phase_Difference(uint32_t *adc_val_buffer1, uint32_t *adc_val_buffer2, float frequency)
{
    if (frequency <= 0.0f) return 0.0f;
    float phase1 = Get_Waveform_Phase(adc_val_buffer1, frequency);
    float phase2 = Get_Waveform_Phase(adc_val_buffer2, frequency);
    float phase_diff = Calculate_Phase_Difference(phase1, phase2);
    return phase_diff;
}

WaveformInfo Get_Waveform_Info(uint32_t *adc_val_buffer_f)
{
    // 【【【 警告修复 】】】 使用 memset 替代 {0}，消除类型混合警告
    WaveformInfo result;
    memset(&result, 0, sizeof(WaveformInfo));
    
    result.vpp = Get_Waveform_Vpp(adc_val_buffer_f, &result.mean, &result.rms);
    result.waveform_type = Analyze_Frequency_And_Type(adc_val_buffer_f, &result.frequency);
    if (result.waveform_type != ADC_WAVEFORM_DC && result.frequency > 0.0f)
    {
        result.phase = Get_Waveform_Phase(adc_val_buffer_f, result.frequency);
        Analyze_Harmonics(adc_val_buffer_f, &result);
    }
    return result;
}

