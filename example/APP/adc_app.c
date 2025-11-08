#include "adc_app.h"
#include "4g_web.h"  // 添加WebSocket通信支持


/*
	1 Roll 轮询获取电压
	2 DMA连续转换求平均值
	3 定时器触发 + DMA + 块处理
	4 DMA 循环 + 半传输中断（HT/TC）双缓冲区实现没有突变的读取正弦波
*/
#define ADC_MODE (3)


#if ADC_MODE == 1


/*
	轮询获取电压
	全部选择disable即可
	唯一注意的是选好分辨率和时钟周期
	决定了采用精读
*/
__IO uint32_t adc_val;		//用于存储计算后的平局ADC值
__IO float voltage;			//英语存储计算后的电压值


void adc_init(void)
{
	/*
	这里不需要初始化
	仅仅是为了多模式下不报错
	*/
}

void adc_task(void)
{
	//第1步  启动ADC转换
	HAL_ADC_Start(&hadc1);
	
	//第2步 等待转换完成（阻塞式）
	//1000ms代表超时时间，超时数据将会直接覆盖
	if(HAL_ADC_PollForConversion(&hadc1,1000) == HAL_OK)
	{
		//第3步 转化成功，读取数字结果 0 - 4095 12位
		adc_val = HAL_ADC_GetValue(&hadc1);
		
		//第4步，将数字值转换为实际的电压值
		//注意查看原理图  Vref的值 假设等于3.3 分辨率12位
		voltage = (float)adc_val * 3.3f / 4096.0f;
		
		//第5步 数据处理
		print("Roll- ADC Value: %lu,  Voltage: %.2f \n\r",adc_val,voltage);
		
	}
	else
	{
		print("ADC Poll TimeOut!");
	}
	
	/*
		第6步
	如果设置为单次转换模式，自动停止，不需要软件停止
	因为是轮询读取，所以配置为单次转换
	如果配置的是连续转换
	需要 HAL_ADC_Stop(&hadc1);软件停止
	*/
//	HAL_ADC_Stop(&hadc1);
}


#elif ADC_MODE == 2
/*
	DMA 连续转换求平均值获取电压
	需要配置DMA循环模式，大小选择- word
	使能DMA请求 和 连续转换
*/

#define ADC_DMA_BUFFER_SIZE 32		//DMA缓冲区元素个数
uint32_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE];	//DMA 目标缓冲区

//__IO代表着这个值外部易变，而且这样定义可以在debug中查看和修改
__IO uint32_t adc_val;		//用于存储计算后的平均数字ADC的值
__IO float voltage;			//用于存储计算后的电压值

void adc_init(void)
{
	//这里不用sizeof(adc_dma_buffer)而是使用ADC_DMA_BUFFER_SIZE
	//因为sizeof获取的是总字节数，而不是元素个数
	HAL_ADC_Start_DMA(&hadc1,adc_dma_buffer,ADC_DMA_BUFFER_SIZE);
}

void adc_task(void)
{
	uint32_t adc_sum = 0;
	
	//第1步 计算MDA缓冲区中所有采样值的总和
	//注意 这里直接读取缓冲区，可能包含不同时刻的采样值
	for(uint16_t i = 0;i < ADC_DMA_BUFFER_SIZE;i++)
	{
		adc_sum += adc_dma_buffer[i];
	}
	
	//第2步 计算平均ADC的值
	adc_val = adc_sum / ADC_DMA_BUFFER_SIZE;
	
	//第3步 将平均数字值转换为实际的电压值
	voltage = (float)adc_val * 3.3f / 4096.0f;
	
	//第4步 处理数据
	print("DMA - ADC Value: %lu,  Voltage: %.2f \n\r",adc_val,voltage);
	
}

//====================ADC_MODE == 3的注释：=======================//
/*
	定时器触发 + DMA + 块处理
	cubemx配置
	使能一个时钟，分频180 重装载 100 频率10000
	每一次时钟溢出，触发“更新事件”
	
	在ADC中继续配置
	规则通道配置
	外部触发ADC转换--定时器3触发事件---上升沿触发
	取消勾选DMA连续转换请求和取消连续转换
	使能ADC中断 （类似使能串口空闲中断要使能串口总中断）
	设置一个合理的采样时间
	
	重要： DMA转换模式配置为 normal（配合中断一般就配置为normal）
	和串口DMA空闲中断一样的配置
	因为在 Circular 模式下，Circular 模式 ≠ 禁用中断！
	{
		在循环模式下，DMA会一直读取
		即使进入中断回调函数的同时，DMA一直在读取
		读取的同时会覆盖原来的值
		直到你打印完之后
		这样同样无法保证从0开始
	}
	
	{
		“DMA 循环 + 半传输中断（HT/TC）就可以完美避免这个问题
		前500个数，我们进行半中断
		这样即使DMA继续读也不会覆盖
		当读到500-1000个数满了
		我继续读取数据，但是只会覆盖0-499
		这样就避免了读写冲突
		
		注意！
		这里有坑
		其实还是无法完美避免
		阻塞式打印时间过长
		DMA连续读取超过500
		这个时候读和写就会冲突
		导致打印的都是0，并且程序卡死
		
		（需要启用ADC的DMA循环以及开启）
	}
	
	
	=================配置多通道模式===========================
	选择同一个ADC的两个不同通道
	打开扫描模式
	分为rank1 rank2分配配置通道和采样时间（触发源相同）
	读取的存储形式为 a0 b0 a1 b1 a2 b2……依次存储到数组
*/


	/*
	
	ADC数据处理函数:
		首先，
	ADC的总转换时间 = 采样时间 (Sampling Time) 和 固定的转换位数时间。

	即Total Conversion Time = （Sampling Time + N ）* (1 / ADCCLK) 
	其中 N 是 ADC 的分辨率 (例如 12 位时 N=12.5 或类似值，具体查阅芯片手册)。
	ADCCLK = （ADC挂载在PB2的外设总线上，为90MHZ，同时ADC配置的4分频，最后是22.5MHZ）
	
	一次数据转换 3 + 12.5 =15.5个ADC时钟周期
	一次数据转换 15.5 / 22.5 = 0.68us
	由此可见，我们设置100us读取一次是完全可以的
	1000个数据转换需要 1000 * 100 = 100 000us = 100ms
	
	*/


	/*
		如果ADC转换过快，即频率过快
	一直使用print打印过快
	就会阻塞系统
	表面上就会发现按键不够灵敏，阻塞了按键
	按键之所以会被阻塞，是因为它被放到调度器里面轮询
	
	解决方案
	把它放到定时器中断里面强制执行
	每10ms强制执行一次
	*/

	//UNUSED 告诉编译器，我知道这个变量存在，但是我不用他
	//也可以使用#define UNUSED(X) ((void)(X))
	//	UNUSED(hadc);
	
			/*
			注意 memeset的单位是字节，而一般API传入的单位是元素个数
			所以使用sizeof时应该谨慎
		元素个数可以直接用宏定义元素数量
		字节数可以用宏定义的元素数量乘以数据类型的字节大小
		*/

#elif ADC_MODE == 3


/*----------------------------------------ADC采样相关变量-------------------------------------------*/
#define BUFFER_SIZE 2048

//将ADC的采样的数据分开两个通道
uint32_t dac_val_buffer[BUFFER_SIZE / 2];		//采集的DAC的采样值
uint32_t res_val_buffer[BUFFER_SIZE / 2];		//采集稳定的电压当做标准电压（改变峰峰值）

//ADC采样的所有数据
__IO uint32_t  adc_val_buffer[BUFFER_SIZE];
//转换为电压
__IO float voltage;


//转换完成标志位
__IO uint8_t AdcConvEnd = 0;

//波形分析标志位
uint8_t wave_analysis_flag = 0;

//波形查询类型			0=全部  1=类型  2 = 频率 3=峰峰值
uint8_t wave_query_type = 0;		


// waveform_analyzer_app.h中声明为extern(在哪里定义，在哪里声明)
WaveformInfo wave_data;





/*----------------------------------------ADC舒适化以及采样解析-------------------------------------------*/

//初始化ADC
void adc_init(void)
{
	HAL_TIM_Base_Start(&htim3);
	HAL_ADC_Start_DMA(&hadc1,(uint32_t *)adc_val_buffer,BUFFER_SIZE);
	__HAL_DMA_DISABLE_IT(&hdma_adc1,DMA_IT_HT);
}

//ADC的DMA转换完缓冲区回调函数---10KHZ  每100us触发一次
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{

	
	if(hadc == &hadc1)
	{
		HAL_ADC_Stop_DMA(hadc);
		AdcConvEnd = 1;
	}
}



/*--------------------------------------ADC数据处理----------------------------------------------------------*/
void adc_task(void)
{
    // 一次数据转换 3(采样) + 12.5(转换) = 15.5 ADC时钟周期
    // 假设 ADC 时钟 14MHz (来自 HSI/PLL), 一次转换时间: 15.5 / 14MHz ~= 1.1 us
    // BUFFER_SIZE 次转换总时间: 1000 * 1.1 us = 1.1 ms (估算值)
    // 定时器触发频率需要匹配这个速率或更慢

    if (AdcConvEnd) // 检查转换完成标志
    {
        // --- 处理采集到的数据 ---
        // 示例：将奇数索引的数据复制到另一个缓冲区 (原因未知，按原逻辑保留)
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            dac_val_buffer[i] = adc_val_buffer[i * 2 + 1]; // 将 ADC 数据存入名为 dac_val_buffer 的数组
            res_val_buffer[i] = adc_val_buffer[i * 2];
        }
        uint32_t res_sum = 0;
        // 将 res_val_buffer 中的数据转换为电压值
        for (uint16_t i = 0; i < BUFFER_SIZE / 2; i++)
        {
            res_sum += res_val_buffer[i];
        }

        uint32_t res_avg = res_sum / (BUFFER_SIZE / 2);
        voltage = (float)res_avg * 3.3f / 4096.0f;
		
        // 将 voltage (0-3.3V) 映射到 DAC 峰值幅度 (0-1650mV，假设 VREF=3.3V)
		/*
			巧妙的通过稳定的电压旋钮来控制DAC的峰值
		DAC输出电压就是以这个为基准的
		
		*/
        dac_app_set_amplitude((uint16_t)(voltage * 1000.0f/2.0f));

        if (uart_send_flag == 1)
        {
            // 通过WebSocket发送ADC电压数据到前端网页
            websocket_send_adc_data(voltage);

            // 可选：发送部分原始采样数据用于详细分析
            // websocket_send_adc_batch(dac_val_buffer, BUFFER_SIZE / 2);
        }

        // 如果设置了波形分析标志，则进行波形分析
        if (wave_analysis_flag)
        {
            wave_data = Get_Waveform_Info(dac_val_buffer);

            // 根据查询类型打印相应的信息
            switch (wave_query_type)
            {
            case 1: // 仅打印波形类型
                my_printf(&huart1, "当前波形类型: %s\r\n", GetWaveformTypeString(wave_data.waveform_type));
                break;

            case 2: // 仅打印频率
                my_printf(&huart1, "当前频率: %dHz\r\n", (uint32_t)wave_data.frequency);
                break;

            case 3: // 仅打印峰峰值
                my_printf(&huart1, "当前峰峰值: %.2fmV\r\n", wave_data.vpp);
                break;
			
			case 4: // 打印全部信息
                my_printf(&huart1, "输入频率: %d FFT频率：%d 峰峰值：%.2f 波形类型：%s\r\n",
                          dac_app_get_update_frequency() / WAVEFORM_SAMPLES,
                          (uint32_t)wave_data.frequency,
                          wave_data.vpp,
                          GetWaveformTypeString(wave_data.waveform_type));
                break;
            }

            wave_analysis_flag = 0; // 分析完成后清除标志
            wave_query_type = 0;    // 重置查询类型
        }

        // --- 处理完成 ---

        // 清空处理缓冲区 (可选，取决于后续逻辑)
        // memset(dac_val_buffer, 0, sizeof(uint32_t) * (BUFFER_SIZE / 2));

        // 清空 ADC DMA 缓冲区和标志位，准备下一次采集
        // memset(adc_val_buffer, 0, sizeof(uint32_t) * BUFFER_SIZE); // 清空原始数据 (如果需要)
        AdcConvEnd = 0;

        // 重新启动 ADC DMA 进行下一次采集
        // 注意：如果定时器是连续触发 ADC 的，可能不需要手动停止/启动 DMA
        // 需要根据 TIM3 和 ADC 的具体配置决定是否需要重新启动
        HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, BUFFER_SIZE);
        __HAL_DMA_DISABLE_IT(&hdma_adc1, DMA_IT_HT); // 再次禁用半传输中断
    }
}


#elif ADC_MODE == 4
/*
	注意！
	1 ADC的DMA的中断优先级要小于串口，以免频繁的DMA数据传输堵塞串口
	2 中断中不能处理过多的东西
	3 中断也不能频繁的调用阻塞式的代码
*/

// ==================== 配置 ====================
#define ADC_BUFFER_SIZE 1000  // 必须为偶数

// ==================== 全局变量 ====================
__IO uint32_t adc_val_buffer[ADC_BUFFER_SIZE];      // DMA 原始缓冲区 [A0,B0,A1,B1,...]
uint32_t dac_val_buffer[ADC_BUFFER_SIZE / 2];       // 处理后的 B 通道数据

volatile uint8_t adc_half_done = 0;   // 半传输完成标志
volatile uint8_t adc_full_done = 0;   // 传输完成标志

// ==================== 初始化 ====================
void adc_init(void)
{
    HAL_TIM_Base_Start(&htim3);
    
    // 启动 Circular DMA
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_val_buffer, ADC_BUFFER_SIZE);
    
    // 启用 HT 和 TC 中断
    __HAL_DMA_ENABLE_IT(&hdma_adc1, DMA_IT_HT | DMA_IT_TC);
}

// ==================== 中断回调：仅设置标志位 ====================
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc == &hadc1)
    {
        adc_half_done = 1;  // 前半段 (0~499) 已就绪
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc == &hadc1)
    {
        adc_full_done = 1;  // 后半段 (500~999) 已就绪
    }
}

// ==================== 主循环任务：安全处理数据 ====================
void adc_task(void)
{
    // 处理前半段
    if (adc_half_done)
    {
        adc_half_done = 0;
        
        // 安全提取前半段 B 通道数据
        for (uint16_t i = 0; i < ADC_BUFFER_SIZE / 4; i++)
        {
            dac_val_buffer[i] = adc_val_buffer[i * 2 + 1];
        }
        
        // 串口打印（主循环中，可阻塞）
        for (uint16_t i = 0; i < ADC_BUFFER_SIZE / 4; i++)
        {
            print("{dac}%d\r\n", (int)dac_val_buffer[i]);
        }
    }

    // 处理后半段
    if (adc_full_done)
    {
        adc_full_done = 0;
        
        // 安全提取后半段 B 通道数据
        for (uint16_t i = 0; i < ADC_BUFFER_SIZE / 4; i++)
        {
            uint32_t src_idx = (ADC_BUFFER_SIZE / 2 + i) * 2 + 1;
            dac_val_buffer[i] = adc_val_buffer[src_idx];
        }
        
        // 串口打印
        for (uint16_t i = 0; i < ADC_BUFFER_SIZE / 4; i++)
        {
            print("{dac}%d\r\n", (int)dac_val_buffer[i]);
        }
    }
}





/*=================================DAC输出正弦波的案例=================================*/
/*
	PA4  DAC输出
	配置DMA 循环模式  16位（uint16_t 查找表，DAC 数据保持寄存器 (DHR) 通常接受 12 位或 8 位写入）
	内存到外设，内存递增（DAC只有一个数据寄存器，每次都写入同一个地址）
	{ADC的配置外设到内存，同样是内存递增，因为每次采集一个电压数据，而内存是一个数组需要递增存储}
	
	配置一个TIM6  10KHZ == 180*100 作为更新事件触发DAC输出
	DAC配置触发源TIM6跟新触发事件（不产生预设波形，自定义）
*/

/*
	关于最后生成的正弦波出现“突变”的问题
	
	DAC100us触发发出一个点，一共100个点，也就是10ms一个周期，
	而DAC是100us读取一个点，DMA一次性读取500个点，
	也就是五个周期的内容，就是50ms，每50ms还要暂停接受，
	进行数据处理，比如串口打印，实际上，串口打印也需要时间，
	而且真正生成图像的就是串口打印的值，在打印的过程中，
	DAC仍然在一直进行输出，以为ADC的DMA配置的是normal,
	但是DAC配置的循环模式，所以这个时候等到串口打印结束，
	ADC继续读取，ADC和DAC此时已经不是同一个0的起跑线了，
	而且这个新的点位是不确定的，所以就出现了突变
	
*/

#define SINE_SAMPLES 100		//一个周期内地采样点数
#define DAC_MAX_VALUE 4095		//12位DAC的最大数字值

uint16_t sine_wave[SINE_SAMPLES];		//存储正弦波数据的数组

// --- 生成正弦波数据的函数 --- 
/*
	查找表说白了就是通过数学方法提前准备好的数组
	数组的每一个值分别对应了正弦波的每个采样点对应的DAC数值
*/

/**
 * @brief 生成正弦波查找表
 * @param buffer:存储波形数据的缓冲区之好着呢
 * @param samples:一个周期内地采样点数
 * @param amplitude：正弦波的峰值幅度
 * @param phase_shif:相位偏移
 * @retval None
*/

void generate_sin_wave(uint16_t * buffer,uint32_t samples,uint16_t amplitude ,float phase_shift)
{
	//把100个采样点映射到0-2π作为角度步进值
	float step = 2.0f * 3.14159f / samples;
	
	for(uint32_t i = 0; i < samples; i++)
	{
		//将角度值映射到正弦值 -1 - 1
		//使用sinf提高效率
		float sine_value = sinf(i * step + phase_shift);
		
		/*
			将正弦值映射到DAC的输出范围：0-4095
		1.0 将-1 - 1 映射到峰值 -2048 -- 2048
		2.0 DAC 是 单极性输出（只能输出 0V ~ 3.3V），不能输出负电压
		所以将幅度上移一个峰值到 0 - 4096
		*/
		
		buffer[i] = (uint16_t)((sine_value * amplitude) +  (DAC_MAX_VALUE / 2.0f));
		
		
		/*
			钳位小技巧
			// 方法 1：手动判断
		value = (value < MIN) ? MIN : (value > MAX ? MAX : value);

		// 方法 2：使用宏
		#define CLAMP(x, low, high) ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))
		value = CLAMP(value, 0, 4095);
		*/
		
		//钳位 确保值在正确的范围内
		if(buffer[i] > DAC_MAX_VALUE)
			buffer[i] = DAC_MAX_VALUE;
//		else if(buffer[i] < 0)
//			buffer[i] = 0;
			
	}
}


/*
	DAC初始化DMA配置
	DMA硬件会自动循环输出波形
*/
void dac_sin_init(void)
{
	//第1步 生成正弦波查找表数据
	generate_sin_wave(sine_wave,SINE_SAMPLES,DAC_MAX_VALUE / 2,0.0f);
	
	//第2步 启动触发DAC的定时器
	HAL_TIM_Base_Start(&htim6);
	
    // 3. 启动 DAC 通道并通过 DMA 输出查找表数据
    //    hdac: DAC 句柄
    //    DAC_CHANNEL_1: 要使用的 DAC 通道
    //    (uint32_t *)SineWave: 查找表起始地址 (HAL 库常需 uint32_t*)
    //    SINE_SAMPLES: 查找表中的点数 (DMA 传输单元数)
    //    DAC_ALIGN_12B_R: 数据对齐方式 (12 位右对齐)
	HAL_DAC_Start_DMA(&hdac,DAC1_CHANNEL_1,(uint32_t*)(sine_wave),SINE_SAMPLES,DAC_ALIGN_12B_R);
	
}






#endif


/*

	1.DA可以发送正弦波 方波 三角波
	2.按键可以控制波形的周期
	3.旋钮可以控制波形的峰峰值
	4.串口最少可以打印出两个周期的波形（可以启动暂停）
	5.可以通过串口查询指令进行参数查询
	可查询参数：
		波形的类型[可以通过DA发送的模式变量去读取、通过AD读取的数据进行分析] 
		频率[不能通过定时器直接得出结果 必须通过AD计算] 
		峰峰值[不能通过定时器直接得出结果 必须通过AD计算]）
	6.可以通过串口控制指令进行模式切换以及参数设置[波形、周期、峰峰值]（具体指令集自定）

*/


