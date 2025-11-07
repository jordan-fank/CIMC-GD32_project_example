#include "4g_web.h"



//新开辟一个环形缓冲区的句柄以及存储缓冲区
struct rt_ringbuffer websocket_ringbuffer;
uint8_t websocket_ringbuffer_pool[128];


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




void websocket_task(void)
{
	uint16_t length;

	length = rt_ringbuffer_data_len(&websocket_ringbuffer);

	if (length == 0)
		return;

	rt_ringbuffer_get(&websocket_ringbuffer, websocket_dma_buffer, length);

	// 调用命令解析函数
	my_printf(&huart1 ,"UASRT6:%s\r\n",websocket_dma_buffer);
//	ws_protocol_parse_buffer((const char *)websocket_dma_buffer, length);
	// 清空接收缓冲区
	memset(websocket_dma_buffer, 0, sizeof(websocket_dma_buffer));
}


