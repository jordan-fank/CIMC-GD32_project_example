/*----------------128×32 0.91寸OLED驱动，基于STM32 HAL库----------*/

#include "oled_app.h"

/*
使用  stdarg  C语言库函数的
va_list, va_start, va_end
可以实现可变参数函数
	“可变参数函数”就是指一个函数能接收不确定数量的参数。
*/

int oled_printf(uint8_t x,uint8_t y, const char *format,...)
{
	//准备一个临时的空缓冲区，用来存储组装好的字符串
	char buffer[128];		//函数内的变量，参数，返回值都存到栈内
	
	//va_list是一个宏，声明一个“魔法工具箱”
	va_list arg;
	int len;
	
	//将所有的参数"..."都装入 arg魔法工具箱里面
	va_start(arg,format);
	
	/*
		vsnprintf的"v"的意识就是va_list
		format就是类似"Data = %d  %d   %d"的字符串
	该函数就是让他使用zheg format 和  arg的工具箱里面装着(1,2,3)
	最后组装成  "Data = 1  2  3"
	这就是巧妙的可变参数函数
	*/
	len = vsnprintf(buffer,sizeof(buffer),format,arg);
	
	
	//清理arg工具箱，防止内存的问题
	va_end(arg);
	
	
	/*
		这里只显示字符，而且字体高度限制为8
		即字体大小限制为  6*8
		而我们的OELD是  128*32
		
		这就意味着我们只能显示4行
		不限制列，意味着你可以自己选择从哪一列开始
	*/
	
	OLED_ShowStr(x , y,(char*)buffer, 8);
	
	return len;
	
}


///**
// * @brief 将 WouoUI 缓冲区的数据发送到 OLED 显示
// * @param buff WouoUI 提供的缓冲区指针，大小为 [高度/8][宽度] 或 [4][128] for 128x32
// */
//static void OLED_SendBuff(uint8_t buff[4][128])
//{  
//    // 遍历每一页 (0-3)
//    for(uint8_t page = 0; page < 4; page++)  
//    {  
//        // 设置 OLED 的页地址
//        OLED_Write_cmd(0xb0 + page); // 0xB0 - 0xB3
//        
//        // 设置 OLED 的列地址 (从第 0 列开始)
//        OLED_Write_cmd(0x00); // 低半字节列地址
//        OLED_Write_cmd(0x10); // 高半字节列地址 (0x10 | 0 = 0)

//        // 循环写入该页的 128 列数据
//        for (uint8_t column = 0; column < 128; column++)  
//        {
//            // 从 WouoUI 缓冲区取数据，并通过基础驱动的数据写入函数发送
//            OLED_Write_data(buff[page][column]); 
//        }
//    } 
//}


//OLED显示任务
void oled_task(void)
{
	/*
	1 显示字符串
	2 显示汉字
	3 显示图片
	*/
	#define disp_mode (1)
	
	
	
	#if disp_mode == 1
	static uint16_t cout = 0;
	//每次执行任务清屏，防止之前的内容保留

	oled_printf(0, 0, "Hello World!!!");
	oled_printf(0, 1, "Welcome to MCU!    ");
	oled_printf(0,3,"Count:%d     ",cout++);
	
	#elif disp_mode == 2
	
//	for(uint8_t i = 0; i<5;i++)
//	{
//		OLED_ShowHanzi(16*i, 0, i);
//	}
//	OLED_ShowHanzi(uint8_t x, uint8_t y, uint8_t no);
//	OLED_ShowHzbig(uint8_t x, uint8_t y, uint8_t n);
	
	for(uint8_t i = 0; i<4;i++)
	{
		OLED_ShowHzbig(32*i,0,i);
	}
	
	#elif disp_mode == 3
	
	//	//显示图片
	OLED_ShowPic(0, 0, 127, 4);
	
	#endif
	
	


}











