/*------------------------------------头文件义------------------------------- */
#include "dac_app.h"


uint32_t initial_frequency = 100;			//初始频率
uint16_t initial_peak_amplitude  = 1000;	//初始峰值幅度



/*------------------------------------全局变量定义------------------------------- */
/*
    这里频繁使用static
    实际上是在模拟C++
    分为公有和私有
*/
// --- Copyright ---
// Copyright (c) 2024 MiCu Electronics Studio. All rights reserved.

// --- 外部 HAL 句柄声明 ---
// 这些句柄通常在 main.c 中定义，需要在这里声明才能使用
// 请确保这些名称与您 CubeMX 生成的或手动定义的名称一致
extern DAC_HandleTypeDef DAC_HANDLE;
extern TIM_HandleTypeDef DAC_TIMER_HANDLE;
extern DMA_HandleTypeDef DAC_DMA_HANDLE;


#if ADC_DAC_SYNC_ENABLE
extern TIM_HandleTypeDef ADC_SYNC_TIMER_HANDLE;
#endif

// --- 私有变量 ---
static uint16_t waveform_buffer[WAVEFORM_SAMPLES];                // 存储当前波形数据的缓冲区
static dac_waveform_t current_waveform = WAVEFORM_SINE;           // 当前输出的波形类型
static uint32_t current_frequency_hz = 1000;                      // 当前波形频率 (Hz)
static uint16_t current_peak_amplitude_mv = DAC_VREF_MV / 2;      // 当前峰值幅度 (mV)
static uint16_t dac_amplitude_raw = DAC_MAX_VALUE / 2;            // 当前峰值幅度对应的 DAC 原始值
static uint8_t adc_sync_enabled = ADC_DAC_SYNC_ENABLE;            // ADC同步状态
static uint8_t adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER; // ADC采样频率倍数
static uint8_t zero_based_waveform = 1;                           // 新增：是否基于零点的波形

// --- 私有函数原型 ---
static void generate_waveform(void);                       // 根据当前设置生成波形数据到缓冲区
static HAL_StatusTypeDef update_timer_frequency(void);     // 更新定时器周期以匹配当前频率
static HAL_StatusTypeDef start_dac_dma(void);              // 启动 DAC DMA 输出
static HAL_StatusTypeDef stop_dac_dma(void);               // 停止 DAC DMA 输出
static void generate_sine(uint16_t amp_raw);               // 生成正弦波数据
static void generate_square(uint16_t amp_raw);             // 生成方波数据
static void generate_triangle(uint16_t amp_raw);           // 生成三角波数据
static HAL_StatusTypeDef update_adc_timer_frequency(void); // 更新ADC定时器频率以匹配同步要求



/*------------------------------------初始化DAC输出------------------------------- */
void app_dac_init(void)
{
	dac_init(initial_frequency,initial_peak_amplitude);	//初始化DAC输出
}




/*------------------------------------波形生成函数------------------------------- */

// 生成正弦波
static void generate_sine(uint16_t amp_raw)
{ 
    float step = 2.0f * 3.14159265f / WAVEFORM_SAMPLES;
    if (zero_based_waveform)
    {
        // 基于零点的正弦波 (0 到 amp_raw*2)
        for (uint32_t i = 0; i < WAVEFORM_SAMPLES; i++)
        {
            float val = (sinf(i * step) + 1.0f) * 0.5f; // 将-1~1映射到0~1
            int32_t dac_val = (int32_t)(val * (amp_raw * 2));
            waveform_buffer[i] = (dac_val < 0) ? 0 : ((dac_val > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)dac_val);
        }
    }
    else
    {
        // 原始实现：基于中点的正弦波
        for (uint32_t i = 0; i < WAVEFORM_SAMPLES; i++)
        {
            float val = sinf(i * step);
            int32_t dac_val = (int32_t)((val * amp_raw) + (DAC_MAX_VALUE / 2.0f));
            waveform_buffer[i] = (dac_val < 0) ? 0 : ((dac_val > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)dac_val);
        }
    }
}

// 生成方波
static void generate_square(uint16_t amp_raw)
{ 
    if (zero_based_waveform)
    {
        // 基于零点的方波 (0 到 amp_raw*2)
        uint16_t high_val = (amp_raw * 2 > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)(amp_raw * 2);
        uint16_t low_val = 0;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        for (uint32_t i = 0; i < half_samples; i++)
            waveform_buffer[i] = high_val;
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
            waveform_buffer[i] = low_val;
    }
    else
    {
        // 原始实现：基于中点的方波
        uint16_t center = DAC_MAX_VALUE / 2;
        int32_t high_val_s = center + amp_raw;
        int32_t low_val_s = center - amp_raw;
        uint16_t high_val = (high_val_s > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)high_val_s;
        uint16_t low_val = (low_val_s < 0) ? 0 : (uint16_t)low_val_s;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        for (uint32_t i = 0; i < half_samples; i++)
            waveform_buffer[i] = high_val;
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
            waveform_buffer[i] = low_val;
    }
}

// 生成三角波
static void generate_triangle(uint16_t amp_raw)
{ 
    if (zero_based_waveform)
    {
        // 基于零点的三角波 (0 到 amp_raw*2)
        uint16_t high_val = (amp_raw * 2 > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)(amp_raw * 2);
        uint16_t low_val = 0;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        float delta_up = (float)(high_val - low_val) / half_samples;
        float delta_down = (float)(high_val - low_val) / (WAVEFORM_SAMPLES - half_samples);

        for (uint32_t i = 0; i < half_samples; i++)
        {
            waveform_buffer[i] = low_val + (uint16_t)(i * delta_up);
        }
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
        {
            waveform_buffer[i] = high_val - (uint16_t)((i - half_samples) * delta_down);
        }
        // 因为斜率是小数，小数点每次都舍去，最后一个点肯定有误差，衔接不好所以这里优化一下
        if (WAVEFORM_SAMPLES > 0)
            waveform_buffer[0] = low_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples > 0)
            waveform_buffer[half_samples - 1] = high_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples < WAVEFORM_SAMPLES)
            waveform_buffer[WAVEFORM_SAMPLES - 1] = low_val;
    }
    else
    {
        // 原始实现：基于中点的三角波
        uint16_t center = DAC_MAX_VALUE / 2;
        int32_t peak = center + amp_raw;
        int32_t trough = center - amp_raw;
        uint16_t high_val = (peak > DAC_MAX_VALUE) ? DAC_MAX_VALUE : (uint16_t)peak;
        uint16_t low_val = (trough < 0) ? 0 : (uint16_t)trough;
        uint32_t half_samples = WAVEFORM_SAMPLES / 2;
        float delta_up = (float)(high_val - low_val) / half_samples;
        float delta_down = (float)(high_val - low_val) / (WAVEFORM_SAMPLES - half_samples);

        for (uint32_t i = 0; i < half_samples; i++)
        {
            waveform_buffer[i] = low_val + (uint16_t)(i * delta_up);
        }
        for (uint32_t i = half_samples; i < WAVEFORM_SAMPLES; i++)
        {
            waveform_buffer[i] = high_val - (uint16_t)((i - half_samples) * delta_down);
        }
        // Ensure start/end points are correct due to potential float rounding
        if (WAVEFORM_SAMPLES > 0)
            waveform_buffer[0] = low_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples > 0)
            waveform_buffer[half_samples - 1] = high_val;
        if (WAVEFORM_SAMPLES > 1 && half_samples < WAVEFORM_SAMPLES)
            waveform_buffer[WAVEFORM_SAMPLES - 1] = low_val;
    }
}



// --- 辅助函数实现--根据当前设置生成波形 ---
static void generate_waveform(void)
{ 
    switch (current_waveform)
    {
    case WAVEFORM_SINE:
        generate_sine(dac_amplitude_raw);
        break;
    case WAVEFORM_SQUARE:
        generate_square(dac_amplitude_raw);
        break;
    case WAVEFORM_TRIANGLE:
        generate_triangle(dac_amplitude_raw);
        break;
    default:
        generate_sine(dac_amplitude_raw);
        break; // 默认为正弦波
    }
}





/*-----------------------------------更新定时器的频率------------------------------- */

//更新DAC定时器的频率
static HAL_StatusTypeDef update_timer_frequency(void)
{ 
    // 检查参数有效性
    if (current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0)
    {
        return HAL_ERROR;
    }

    // 计算DAC更新频率
    /*我们想生成1KHZ的波形，那就是1秒输出1000个波形完整周期
        但是我们不能简单定义定时器1KHZ
        因为它只是它每一毫秒只生成一个点，而不是一个波形
        实际上是256个点才是一个波形
        所以需要定时器的频率应该设置为1000*256，更快
    */
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    
    // 获取定时器时钟频率（直接使用TIMER_INPUT_CLOCK_HZ）
    uint32_t timer_clk = TIMER_INPUT_CLOCK_HZ;
    
    // 对于高频率，使用简单直接的方法计算ARR和PSC
    uint32_t prescaler = 0; // 优先使用无分频
    uint32_t arr = timer_clk / dac_update_freq - 1;
    
    // 如果ARR超出范围，再计算合适的分频
    if (arr > 0xFFFF)
    {
       /*
        加1是为了向上取整，除法是直接去掉小数部分
        显然这样最后得到的ARR会略大于上限值
        其次我们计算的是实际的分频系数
        而PSC寄存器值==实际的分频系数 - 1
        所以这里加一  减1  是符合逻辑的
       */
        uint32_t div_factor = (arr / 0xFFFF) + 1;
        prescaler = div_factor - 1;
        arr = timer_clk / (dac_update_freq * (prescaler + 1)) - 1;
    }
    
    // 确保ARR至少为1
    if (arr == 0)
    {
        arr = 1;
    }
    
    // 应用设置
    __HAL_TIM_SET_PRESCALER(&DAC_TIMER_HANDLE, prescaler);
    __HAL_TIM_SET_AUTORELOAD(&DAC_TIMER_HANDLE, arr);
    
    // 生成更新事件，立即加载新值
    HAL_TIM_GenerateEvent(&DAC_TIMER_HANDLE, TIM_EVENTSOURCE_UPDATE);
    
    // 如果ADC同步已启用，则同时更新ADC定时器频率
    if (adc_sync_enabled)
    {
        update_adc_timer_frequency();
    }

    return HAL_OK;
}




/*------------------------------------DAC启动和停止------------------------------- */


// 停止 DAC DMA 和定时器
static HAL_StatusTypeDef stop_dac_dma(void) 
{
    HAL_StatusTypeDef status1 = HAL_DAC_Stop_DMA(&DAC_HANDLE, DAC_CHANNEL);
    HAL_TIM_Base_Stop(&DAC_TIMER_HANDLE); // 直接使用 Stop，不检查返回值简化
    return status1;                       // 主要关心 DAC DMA 是否成功停止
}


// 启动 DAC DMA 和定时器
static HAL_StatusTypeDef start_dac_dma(void)
{                                                            
    HAL_StatusTypeDef status_tim = update_timer_frequency(); // 确保定时器频率正确
    if (status_tim != HAL_OK)
        return status_tim; // 如果频率设置失败则不启动

    HAL_StatusTypeDef status_dac = HAL_DAC_Start_DMA(&DAC_HANDLE, DAC_CHANNEL, (uint32_t *)waveform_buffer, WAVEFORM_SAMPLES, DAC_ALIGN_12B_R);
    HAL_StatusTypeDef status_tim_start = HAL_TIM_Base_Start(&DAC_TIMER_HANDLE);

    return (status_dac == HAL_OK && status_tim_start == HAL_OK) ? HAL_OK : HAL_ERROR;
}

//DAC初始化（公有）
void dac_init(uint32_t initial_freq_hz, uint16_t initial_peak_amplitude_mv)
{
    current_frequency_hz = (initial_freq_hz == 0) ? 1 : initial_freq_hz; // 避免频率为0
    dac_app_set_amplitude(initial_peak_amplitude_mv);                    // 设置初始幅度 (内部会计算 dac_amplitude_raw)
    generate_waveform();                                                 // 生成初始波形数据

    // 初始化ADC同步配置
    adc_sync_enabled = ADC_DAC_SYNC_ENABLE;
    adc_sampling_multiplier = ADC_SAMPLING_MULTIPLIER;

    start_dac_dma(); // 启动定时器和 DMA 输出
}




/*------------------------------------ADC同步相关------------------------------- */
/*
    采样频率必须至少是信号最高频率的 2 倍
    实际应用中，我们通常需要 5-10 倍甚至更高，才能得到足够精细的波形细节
*/

//设置ADC定时器的频率
static HAL_StatusTypeDef update_adc_timer_frequency(void)
{
    //如果需要ADC和DAC的同步功能
#if ADC_DAC_SYNC_ENABLE
    if (current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0 || TIMER_INPUT_CLOCK_HZ == 0 || adc_sampling_multiplier == 0)
    {
        return HAL_ERROR; // 参数无效
    }

    // 计算ADC采样频率，是DAC更新频率的倍数
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    uint64_t adc_sample_freq = dac_update_freq * adc_sampling_multiplier;

    // 确保不超过硬件限制
    if (adc_sample_freq > TIMER_INPUT_CLOCK_HZ)
    {
        adc_sample_freq = TIMER_INPUT_CLOCK_HZ;
    }

    // 计算ADC定时器的周期值
    uint32_t tim_period = (uint32_t)(TIMER_INPUT_CLOCK_HZ / adc_sample_freq);
    if (tim_period == 0)
        tim_period = 1;
    if (tim_period > 0xFFFF)
        tim_period = 0xFFFF;

    //修改前先关闭定时器
    HAL_TIM_Base_Stop(&ADC_SYNC_TIMER_HANDLE);

    __HAL_TIM_SET_PRESCALER(&ADC_SYNC_TIMER_HANDLE, 0); // 不分频
    __HAL_TIM_SET_AUTORELOAD(&ADC_SYNC_TIMER_HANDLE, tim_period - 1);

    //立即更新定时器的配置
    HAL_TIM_GenerateEvent(&ADC_SYNC_TIMER_HANDLE, TIM_EVENTSOURCE_UPDATE);


    //修改后开启定时器
    HAL_TIM_Base_Start(&ADC_SYNC_TIMER_HANDLE);

    return HAL_OK;
#else
    return HAL_OK; // 如果未启用同步功能，直接返回成功
#endif
}


//设置DAC的同步使能和采用倍数 （公有）
HAL_StatusTypeDef dac_app_config_adc_sync(uint8_t enable, uint8_t multiplier)
{
    if (multiplier == 0)
    {
        return HAL_ERROR; // 倍数不能为0
    }

    // 存储新的配置
    adc_sync_enabled = enable;
    adc_sampling_multiplier = multiplier;

    // 如果启用同步，则立即更新ADC定时器配置
    if (enable)
    {
        return update_adc_timer_frequency();
    }

    return HAL_OK;
}




/*------------------------------------波形相关参数设置------------------------------- */

/*
    在修改波形参数 类型，频率 幅度之前
    都要先停止DAM和定时器，在重新启动
    如果此时DMA正在运行
    就可能会读到正在被修改的，不完整的数据，导致波形错误

    修改定时器频率也应该注意
    如果定时器正在运行，新的配置不会立即生效，或者产生一个不稳定的过渡期
*/

// 设置波形类型（公有）
HAL_StatusTypeDef dac_app_set_waveform(dac_waveform_t type) 
{
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;
    current_waveform = type;
    generate_waveform(); // 使用新的类型和当前的幅度重新生成波形
    // 频率不变，定时器周期不需要重新计算，只需重新启动
    return start_dac_dma(); // 如果启用了ADC同步，会在start_dac_dma中调用update_adc_timer_frequency
}

// 设置DAC触发定时器的频率 （公有）

HAL_StatusTypeDef dac_app_set_frequency(uint32_t freq_hz) 
{
    if (freq_hz == 0)
        return HAL_ERROR; // 不允许频率为 0
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;
    current_frequency_hz = freq_hz;
    // 波形类型和幅度不变，只需更新定时器并重启
    return start_dac_dma(); // start_dac_dma 内部会调用 update_timer_frequency和update_adc_timer_frequency
}



/*
	//峰值幅度是相对于中心电压而言的
	所以设置的峰值要针对中心电压进行压缩
	
*/
//设置波形峰值幅度 （公有）
HAL_StatusTypeDef dac_app_set_amplitude(uint16_t peak_amplitude_mv) 
{
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;

    uint16_t max_amplitude_mv = DAC_VREF_MV / 2;
    current_peak_amplitude_mv = (peak_amplitude_mv > max_amplitude_mv) ? max_amplitude_mv : peak_amplitude_mv;

    // 将 mV 幅度转换为 DAC 原始步进值 (相对于中心值 DAC_MAX_VALUE / 2)
    //设置两个浮点数相除，最后得到一个浮点数
    dac_amplitude_raw = (uint16_t)(((float)current_peak_amplitude_mv / max_amplitude_mv) * (DAC_MAX_VALUE / 2.0f));
    // 再次钳位确保不超过理论最大值 (虽然理论上不会)
    if (dac_amplitude_raw > (DAC_MAX_VALUE / 2))
        dac_amplitude_raw = DAC_MAX_VALUE / 2;

    generate_waveform(); // 使用新的幅度和当前的类型重新生成波形
    // 频率不变，ADC同步将在start_dac_dma中处理
    return start_dac_dma();
}




/*-------------------------------获取DAC同步配置相关参数------------------------------*/


// 获取计算出的ADC采样间隔 （公有）

float dac_app_get_adc_sampling_interval_us(void)
{ 
    if (!adc_sync_enabled || current_frequency_hz == 0 || WAVEFORM_SAMPLES == 0 || adc_sampling_multiplier == 0)

    {
        return 0.0f; // 如果未启用同步或参数无效, 返回0
    }
    uint64_t dac_update_freq = (uint64_t)current_frequency_hz * WAVEFORM_SAMPLES;
    uint64_t adc_sample_freq = dac_update_freq * adc_sampling_multiplier;
    if (adc_sample_freq == 0)

    {
        return 0.0f;
    }

    return 1000000.0f / (float)adc_sample_freq;
}

// 设置波形基准模式   1 零点  0 中点 （公有）

HAL_StatusTypeDef dac_app_set_zero_based(uint8_t enable)
{
    if (stop_dac_dma() != HAL_OK)
        return HAL_ERROR;

    zero_based_waveform = enable ? 1 : 0;

    generate_waveform(); // 使用新的模式重新生成波形
    return start_dac_dma();
}

//获取DAC每秒更新点数，用于外部ADC同步配置 （公有）

uint32_t dac_app_get_update_frequency(void)
{

    return current_frequency_hz * WAVEFORM_SAMPLES;
}

// 获取当前峰值幅度 （公有）

uint16_t dac_app_get_amplitude(void)
{
    return current_peak_amplitude_mv;
}

// 获取当前波形基准模式 1零点或者0中点 （公有）

uint8_t dac_app_get_zero_based(void)
{
    return zero_based_waveform;
}


