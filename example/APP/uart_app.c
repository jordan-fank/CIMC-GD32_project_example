#include "uart_app.h"


/*
	1 串口中断+超时解析
	2 DMA+空闲中断
	3 DMA空闲中断+环形缓冲区
	
	注意：如果需要在头文件中使用
	这行代码就定义在main.h中 公用
*/
#define UART_MODE (3)


#if UART_MODE == 1

/*---------------方法一 串口中断+超时解析-----------------*/
/*
	cubemx配置好波特率，使能中断即可（异步模式）
	关于使用HAL_UART_Receive_IT(&huart1, &uart_rx_buffer[uart_rx_index], 1);这种形式
	依赖 HAL 的多字节接收机制
	必须重置指针huart1.pRxBuffPtr = uart_rx_buffer;
	
	如果不依赖这种：
	 // 重新启动接收（始终接收 1 字节到 uart_rx_byte）
        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
		uart_rx_buffer[uart_rx_index++] = uart_rx_byte;
		uart_rx_buffer[uart_rx_index] = '\0'; // 确保字符串结尾

*/
uint8_t uart_rx_byte;	//每次接受一个字节
uint16_t uart_rx_index;	//存储指针
uint32_t uart_rx_ticks;	//超时计时
uint8_t uart_rx_buffer[128];	//存储区

#define UART_TIMEOUT_MS 100			//超时解析的超时时间


//串口初始化
void uart_init(void)
{
	HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
}

//串口接受中断回调函数--超时解析
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//第一步 确定串口
	if(huart->Instance == USART1)
	{
		//第二步 记录接受时间
		uart_rx_ticks = uwTick;		//记住最后接受数据的时间
		
		//第三步 增加存储指针
		if (uart_rx_index < sizeof(uart_rx_buffer) - 1)
        {
            uart_rx_buffer[uart_rx_index++] = uart_rx_byte;
        }
		
		//第四步 重启接收中断
		HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
	}
}


void uart_task(void)
{
	//第一步 如果存储指针为0 说明没有数据需要处理
	if(uart_rx_index == 0)
		return;
	
	
	//第二步  超时解析
	if(uwTick - uart_rx_ticks > UART_TIMEOUT_MS)
	{
		//第三步 确保字符串结尾
		 uart_rx_buffer[uart_rx_index] = '\0'; 
		
		//第四步 处理数据
		print("Over time uart data : %s \n\r",uart_rx_buffer);
		
		
		//第五步 清理缓冲区和存储指针
		memset(uart_rx_buffer,0,uart_rx_index);
		uart_rx_index = 0;
		

	}		
}


#elif UART_MODE == 2

/*-----------------------方法二 DMA+空闲中断-----------------------*/
/*
	使能DMA normal模式 字节
	使能中断
*/

uint8_t uart_rx_dma_buffer[128];	//DMA接受的缓冲区
uint8_t uart_dma_buffer[128];		//复制到待处理的缓冲区
uint8_t uart_flag;					//处理数据标志位



//串口初始化
void uart_init(void)
{
	 HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
	 __HAL_DMA_DISABLE_IT(&hdma_usart1_rx ,DMA_IT_HT);
}


//DMA空闲中断回调函数
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	//第一步 确认是目标串口
	if(huart->Instance == USART1)
	{
		//第二步 停止DMA传输，空闲中断意味着发送停止，所以停止DMA等待
		HAL_UART_DMAStop(huart);
		
		//第三步 讲DMA缓冲区的有效数据Size字节，复制到待处理缓冲区
		memcpy(uart_dma_buffer,uart_rx_dma_buffer,Size);
		
		//第四步 数据处理标志位置一，告诉主循环有数据待处理
		uart_flag = 1;
		
		//第五步  清空DMA接收缓冲区，未下次做准备
		memset(uart_rx_dma_buffer,0,sizeof(uart_rx_dma_buffer));
		
		//第六步 重新启动下一次DMA空闲接受中断
		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
		
		//第七步 关闭DMA半满中断 
		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

		
	}
}

void uart_task(void)
{
	//第一步，如果数据处理标志位为0 说明没有数据需要处理，直接返回
	if(uart_flag == 0)
		return;
	
	//第二步，清空标志位
	uart_flag = 0;
	
	//第三步 处理数据
	print("DMA uart data : %s \r\n",uart_dma_buffer);
	
	//第四步  清空缓冲区 将接收所有置零
	memset(uart_dma_buffer,0,sizeof(uart_dma_buffer));
	
}


#elif UART_MODE == 3

/*-----------------------方法三 DMA空闲中断 + 环形缓冲区-----------------------*/

/*
	使能DMA normal模式 字节
	使能中断
*/

/*================================串口接受相关变量=======================*/
uint8_t uart_rx_dma_buffer[128];	//DMA硬件接受的缓冲区
uint8_t uart_dma_buffer[128];		//复制到待处理的缓冲区

struct rt_ringbuffer uart_ringbuffer;		//环形缓冲区结构体（接口）
uint8_t ringbuffer_pool[128];				//环形缓冲区内存（数据中转站--先在中断回调放进去，然后在数据解析中取出来）


/*================================串口处理外部信息相关变量=======================*/
dac_waveform_t current_waveform_uart;




/*===========================串口解析模块========================*/
//串口初始化
void uart_init(void)
{
	 HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
	 __HAL_DMA_DISABLE_IT(&hdma_usart1_rx ,DMA_IT_HT);
	
	rt_ringbuffer_init(&uart_ringbuffer,ringbuffer_pool,sizeof(ringbuffer_pool));
}


void UART1_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    HAL_UART_DMAStop(huart);
	
    rt_ringbuffer_put(&uart_ringbuffer, uart_rx_dma_buffer, Size);
    memset(uart_rx_dma_buffer, 0, sizeof(uart_rx_dma_buffer));
	
    HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
    __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);
}

////DMA空闲中断回调函数
//void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
//{
//	//第一步 确认是目标串口
//	if(huart->Instance == USART1)
//	{
//		//第二步 停止DMA传输，空闲中断意味着发送停止，所以停止DMA等待
//		HAL_UART_DMAStop(huart);
//		
//		//第三步 将DMA接受到的数据，放到环形环形缓冲区的池子里面，大小为Size
//		rt_ringbuffer_put(&uart_ringbuffer,uart_rx_dma_buffer,Size);

//		
//		//第四步  清空DMA接收缓冲区，未下次做准备
//		memset(uart_rx_dma_buffer,0,sizeof(uart_rx_dma_buffer));
//		
//		//第五步 重新启动下一次DMA空闲接受中断
//		HAL_UARTEx_ReceiveToIdle_DMA(&huart1, uart_rx_dma_buffer, sizeof(uart_rx_dma_buffer));
//		
//		//第六步 关闭DMA半满中断 
//		__HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

//		
//	}
//}




//串口命令解析函数
void parse_uart_command(uint8_t *buffer,uint16_t length)
{
	/*
	大部分字符串操作符都是一直读到空字符"\0"为止
	比如printf  strcmp  strncmp
	注意双引号""后面必定多一个‘\0’
	而单引号''如果是空的，结果就是空的
	
	注意"\0"和 '\0'的区别，前者其实是两个字符都是'\0'
	*/
	
	/*
		大部分关于字符的操作都是一直查到空字符串停止
		比如printf,strcmp  strncmp
	*/
	// 确保缓冲区末尾有字符串结束符
	if (length < sizeof(uart_dma_buffer))
		buffer[length] = '\0';
	else
		buffer[sizeof(uart_dma_buffer) - 1] = '\0';

		

	// 查询指令解析
	if (strncmp((char *)buffer, "GET:", 4) == 0)
	{
		if (strncmp((char *)buffer + 4, "TYPE", 4) == 0)
		{
			wave_analysis_flag = 1;	// 触发FFT分析
			wave_query_type = 1;	// 查询波形类型
			
			
			// 在下次ADC采集完成时会更新wave_data
			my_printf(&huart1, "FFT分析中\r\n");
		}
		else if (strncmp((char *)buffer + 4, "FREQ", 4) == 0)
		{
			wave_analysis_flag = 1; // 触发FFT分析
			wave_query_type = 2;	// 查询频率
			
			
			// 在下次ADC采集完成时会更新wave_data
			my_printf(&huart1, "FFT分析中\r\n");
		}
		else if (strncmp((char *)buffer + 4, "AMP", 3) == 0)
		{
			wave_analysis_flag = 1; // 触发FFT分析
			wave_query_type = 3;	// 查询峰峰值
			
			
			// 在下次ADC采集完成时会更新wave_data
			my_printf(&huart1, "FFT分析中\r\n");
		}
		else if (strncmp((char *)buffer + 4, "ALL", 3) == 0)
		{
			wave_analysis_flag = 1; // 触发FFT分析
			wave_query_type = 0;	// 查询所有参数
			
			
			// 在下次ADC采集完成时会更新wave_data
			my_printf(&huart1, "FFT分析中\r\n");
		}
		else
		{
			my_printf(&huart1, "错误: 未知的查询参数\r\n");
		}
	}
	
	// 设置 指令解析
	else if (strncmp((char *)buffer, "SET:", 4) == 0)
	{
		char *param_str = (char *)buffer + 4;
		char *value_str = strchr(param_str, ':');	//strchr返回某个字符在字符串中的位置

		if (value_str == NULL)
		{
			my_printf(&huart1, "错误: 设置指令格式错误\r\n");
			return;
		}

		// 将值字符串中的冒号替换为结束符，以分隔参数名和值
		*value_str = '\0';
		value_str++; // 指向值字符串的开始
		
		

		if (strcmp(param_str, "TYPE") == 0)
		{
			// 设置波形类型--将字符串转化为整数（atoi）
			int type = atoi(value_str);
			
			if (type >= 0 && type <= 2) // 检查类型是否有效
			{
				HAL_StatusTypeDef status = dac_app_set_waveform((dac_waveform_t)type);
				
				if (status == HAL_OK)
				{
					my_printf(&huart1, "波形类型已设置\r\n");
				}
				else
				{
					my_printf(&huart1, "设置波形类型失败\r\n");
				}
			}
			else
			{
				my_printf(&huart1, "错误: 无效的波形类型 (0-2)\r\n");
			}
		}
		
		else if (strcmp(param_str, "FREQ") == 0)
		{
			// 设置频率
			uint32_t freq = atoi(value_str);
			if (freq > 0) // 频率必须大于0
			{
				HAL_StatusTypeDef status = dac_app_set_frequency(freq);
				
				if (status == HAL_OK)
				{
					my_printf(&huart1, "频率已设置为: %dHz\r\n", freq);
				}
				else
				{
					my_printf(&huart1, "设置频率失败\r\n");
				}
			}
			else
			{
				my_printf(&huart1, "错误: 无效的频率值\r\n");
			}
		}
		else if (strcmp(param_str, "AMP") == 0)
		{
			// 设置峰峰值
			uint16_t vpp = atoi(value_str);
			// 将峰峰值转换为峰值（÷2）
			uint16_t peak_amp = vpp / 2;

			if (peak_amp > 0 && peak_amp <= (DAC_VREF_MV / 2)) // 检查峰值是否在有效范围内
			{
				HAL_StatusTypeDef status = dac_app_set_amplitude(peak_amp);
				
				if (status == HAL_OK)
				{
					my_printf(&huart1, "峰峰值已设置为: %dmV\r\n", vpp);
				}
				else
				{
					my_printf(&huart1, "设置峰峰值失败\r\n");
				}
			}
			else
			{
				my_printf(&huart1, "错误: 无效的峰峰值 (0-%d)\r\n", DAC_VREF_MV);
			}
		}
		else
		{
			my_printf(&huart1, "错误: 未知的设置参数\r\n");
		}
	}
	else
	{
		// 不是有效的GET或SET命令
		if (length > 1) // 避免空命令
		{
			my_printf(&huart1, "错误: 未知命令\r\n");
			my_printf(&huart1, "有效命令:\r\n");
			my_printf(&huart1, "GET:TYPE - 查询波形类型\r\n");
			my_printf(&huart1, "GET:FREQ - 查询频率\r\n");
			my_printf(&huart1, "GET:AMP - 查询峰峰值\r\n");
			my_printf(&huart1, "SET:TYPE:x - 设置波形类型(0=正弦波,1=方波,2=三角波)\r\n");
			my_printf(&huart1, "SET:FREQ:x - 设置频率(Hz)\r\n");
			my_printf(&huart1, "SET:AMP:x - 设置峰峰值(mV)\r\n");
		}
	}
	
}




void uart_task(void)
{
	//第一步 获取环形缓冲区的大小 若为空，不处理
	uint16_t length;
	length =  rt_ringbuffer_data_len(&uart_ringbuffer);
	if(length == 0) return;
	
	
	//第二步，从环形缓冲区里面取出数据到另外一个数组里面进行数据解析
	rt_ringbuffer_get(&uart_ringbuffer,uart_dma_buffer,length);
	
	// 第三步 调用命令解析函数
//	parse_uart_command(uart_dma_buffer, length);
	
//	my_printf(&huart1 ,"UASRT1:%s\r\n",uart_dma_buffer);
	shell_process(uart_dma_buffer, length);
	
	//第四步  清空缓冲区 将接收所有置零
	memset(uart_dma_buffer,0,sizeof(uart_dma_buffer));
	
}





#endif			//结束USART的三种通讯模式






/*==============================串口重定向==========================*/
/*
串口重定向
当使用DMA时，就不使用传统的串口重定向
就使用先进的的串口重定向

打开微库
不使用半主机模式，所以必须重定向
*/

int my_printf(UART_HandleTypeDef *huart, const char *format, ...)
{
	static char buffer[512];
	va_list arg;
	int len;
	// 初始化可变参数列表
	va_start(arg, format);
	len = vsnprintf(buffer, sizeof(buffer), format, arg);
	va_end(arg);
	HAL_UART_Transmit(huart, (uint8_t *)buffer, (uint16_t)len, 0xFF);
	return len;
}





