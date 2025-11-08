#include "4g_web.h"
#include "led_app.h"  // 添加LED控制头文件



//新开辟一个环形缓冲区的句柄以及存储缓冲区
struct rt_ringbuffer websocket_ringbuffer;
uint8_t websocket_ringbuffer_pool[128];

// WebSocket LED控制标志 (0=自动模式, 1=手动控制模式)
static uint8_t websocket_led_control_mode = 0;

//串口接受的数据存储缓冲区和数据处理缓冲区
uint8_t websocket_rx_dma_buffer[128] = {0};		//从websocket接受的数据
uint8_t websocket_dma_buffer[128] = {0};		//讲接受的数据复制下来进行处理



/*===========================串口解析模块========================*/
//串口初始化
void websocket_init(void)
{
	 HAL_UARTEx_ReceiveToIdle_DMA(&huart6, websocket_rx_dma_buffer, sizeof(websocket_rx_dma_buffer));
	 __HAL_DMA_DISABLE_IT(&hdma_usart6_rx ,DMA_IT_HT);

	rt_ringbuffer_init(&websocket_ringbuffer,websocket_ringbuffer_pool,sizeof(websocket_ringbuffer_pool));
}

void UART6_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    HAL_UART_DMAStop(huart);
    rt_ringbuffer_put(&websocket_ringbuffer, websocket_rx_dma_buffer, Size);
    memset(websocket_rx_dma_buffer, 0, sizeof(websocket_rx_dma_buffer));
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, websocket_rx_dma_buffer, sizeof(websocket_rx_dma_buffer));
    __HAL_DMA_DISABLE_IT(&hdma_usart6_rx, DMA_IT_HT);
}


void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART1)
    {
        UART1_RxEventCallback(huart, Size);
    }
    else if (huart->Instance == USART6)
    {
        UART6_RxEventCallback(huart, Size);
    }
}


/**
 * @brief WebSocket LED控制命令解析函数
 * @param buffer 接收到的数据缓冲区
 * @param length 数据长度
 * @note 解析格式: "LED1_ON" (LED1开启), "LED1_OFF" (LED1关闭)
 */
void websocket_parse_led_command(uint8_t *buffer, uint16_t length)
{
    // 确保缓冲区末尾有字符串结束符
    if (length >= sizeof(websocket_dma_buffer))
        return;

    buffer[length] = '\0';

    // 打印接收到的原始数据用于调试
    my_printf(&huart1, "[WebSocket] 收到数据: %s\r\n", (char*)buffer);

    // 【关键修复】过滤响应消息，防止无限循环
    // 如果消息包含" OK"，说明是响应消息，直接忽略，不作为命令处理
    char *ok_pos = strstr((char*)buffer, " OK");
    if (ok_pos != NULL) {
        my_printf(&huart1, "[WebSocket] 检测到响应消息，忽略处理: %s\r\n", (char*)buffer);
        return;
    }

    // 解析LED1_ON命令
    if (strncmp((char*)buffer, "LED1_ON", 7) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[0] = 1; // LED1开启
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED1开启\r\n");
        websocket_send_led_response(1, 1);
        return;
    }

    // 解析LED1_OFF命令
    if (strncmp((char*)buffer, "LED1_OFF", 8) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[0] = 0; // LED1关闭
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED1关闭\r\n");
        websocket_send_led_response(1, 0);
        return;
    }

    // 解析LED2_ON命令
    if (strncmp((char*)buffer, "LED2_ON", 7) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[1] = 1; // LED2开启
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED2开启\r\n");
        websocket_send_led_response(2, 1);
        return;
    }

    // 解析LED2_OFF命令
    if (strncmp((char*)buffer, "LED2_OFF", 8) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[1] = 0; // LED2关闭
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED2关闭\r\n");
        websocket_send_led_response(2, 0);
        return;
    }

    // 解析LED3_ON命令
    if (strncmp((char*)buffer, "LED3_ON", 7) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[2] = 1; // LED3开启
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED3开启\r\n");
        websocket_send_led_response(3, 1);
        return;
    }

    // 解析LED3_OFF命令
    if (strncmp((char*)buffer, "LED3_OFF", 8) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[2] = 0; // LED3关闭
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED3关闭\r\n");
        websocket_send_led_response(3, 0);
        return;
    }

    // 解析LED4_ON命令
    if (strncmp((char*)buffer, "LED4_ON", 7) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[3] = 1; // LED4开启
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED4开启\r\n");
        websocket_send_led_response(4, 1);
        return;
    }

    // 解析LED4_OFF命令
    if (strncmp((char*)buffer, "LED4_OFF", 8) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[3] = 0; // LED4关闭
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED4关闭\r\n");
        websocket_send_led_response(4, 0);
        return;
    }

    // 解析LED5_ON命令
    if (strncmp((char*)buffer, "LED5_ON", 7) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[4] = 1; // LED5开启
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED5开启\r\n");
        websocket_send_led_response(5, 1);
        return;
    }

    // 解析LED5_OFF命令
    if (strncmp((char*)buffer, "LED5_OFF", 8) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[4] = 0; // LED5关闭
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED5关闭\r\n");
        websocket_send_led_response(5, 0);
        return;
    }

    // 解析LED6_ON命令
    if (strncmp((char*)buffer, "LED6_ON", 7) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[5] = 1; // LED6开启
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED6开启\r\n");
        websocket_send_led_response(6, 1);
        return;
    }

    // 解析LED6_OFF命令
    if (strncmp((char*)buffer, "LED6_OFF", 8) == 0)
    {
        websocket_led_control_mode = 1;
        extern uint8_t ucLed[6];
        ucLed[5] = 0; // LED6关闭
        extern void led_disp(uint8_t *ucLed);
        led_disp(ucLed);
        my_printf(&huart1, "[WebSocket] LED6关闭\r\n");
        websocket_send_led_response(6, 0);
        return;
    }

    // 未知命令
    if (length > 1) {
        my_printf(&huart1, "[WebSocket] 未知命令: %s\r\n", (char*)buffer);
    }
}

/**
 * @brief 发送LED控制响应给WebSocket客户端
 * @param led_num LED编号
 * @param state LED状态 (1=开启, 0=关闭)
 */
void websocket_send_led_response(uint8_t led_num, uint8_t state)
{
    char response[16];
    int len;

    // 构建响应字符串：LED1_ON OK 或 LED1_OFF OK
    if (state == 1) {
        len = snprintf(response, sizeof(response), "LED%d_ON OK", led_num);
    } else {
        len = snprintf(response, sizeof(response), "LED%d_OFF OK", led_num);
    }

    // 通过串口6发送响应给WebSocket模块
    HAL_UART_Transmit(&huart6, (uint8_t*)response, len, 100);

    my_printf(&huart1, "[WebSocket] 发送响应: %s\r\n", response);
}

/**
 * @brief 发送ADC电压数据给WebSocket客户端
 * @param voltage ADC电压值 (浮点数)
 */
void websocket_send_adc_data(float voltage)
{
    char adc_msg[32];
    int len;

    // 构建ADC数据消息：ADC:2.35
    len = snprintf(adc_msg, sizeof(adc_msg), "ADC:%.2f", voltage);

    // 通过串口6发送ADC数据给WebSocket模块
    HAL_UART_Transmit(&huart6, (uint8_t*)adc_msg, len, 100);

    my_printf(&huart1, "[WebSocket] 发送ADC数据: %s\r\n", adc_msg);
}

/**
 * @brief 批量发送ADC采样数据给WebSocket客户端
 * @param adc_data ADC采样数据数组
 * @param count 数据个数
 */
void websocket_send_adc_batch(uint32_t *adc_data, uint16_t count)
{
    char adc_msg[64];

    // 发送一批ADC数据，每条消息格式：ADC:1234
    for (uint16_t i = 0; i < count && i < 10; i++) {  // 限制每次最多发送10个数据点
        int len = snprintf(adc_msg, sizeof(adc_msg), "ADC:%d", (int)adc_data[i]);

        // 通过串口6发送ADC数据给WebSocket模块
        HAL_UART_Transmit(&huart6, (uint8_t*)adc_msg, len, 100);

        // 短暂延时避免发送过快
        HAL_Delay(1);
    }

    my_printf(&huart1, "[WebSocket] 批量发送ADC数据，数量: %d\r\n", count);
}

/**
 * @brief 获取LED控制模式
 * @return 0=自动模式, 1=手动控制模式
 */
uint8_t websocket_get_led_control_mode(void)
{
    return websocket_led_control_mode;
}



void websocket_task(void)
{
	uint16_t length;

	length = rt_ringbuffer_data_len(&websocket_ringbuffer);


	if (length == 0)
		return;

	rt_ringbuffer_get(&websocket_ringbuffer, websocket_dma_buffer, length);

	// 调用WebSocket LED控制命令解析函数
	websocket_parse_led_command(websocket_dma_buffer, length);

    
	// 清空接收缓冲区
	memset(websocket_dma_buffer, 0, sizeof(websocket_dma_buffer));
}


