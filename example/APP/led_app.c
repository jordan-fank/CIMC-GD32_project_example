#include "led_app.h"


uint8_t ucLed[6] = {0,0,0,0,0,0};  // LED 状态数组 (6个LED)

/**
 * @brief 根据ucLed数组状态更新6个LED的显示
 * @param ucLed Led数据储存数组 (大小为6)
 */
void led_disp(uint8_t *ucLed)
{
    uint8_t temp = 0x00;                // 用于记录当前 LED 状态的临时变量 (最低6位有效)
    static uint8_t temp_old = 0xff;     // 记录之前 LED 状态的变量, 用于判断是否需要更新显示

    for (int i = 0; i < 6; i++)         // 遍历6个LED的状态
    {
        // 将LED状态整合到temp变量中，方便后续比较
        if (ucLed[i]) temp |= (1 << i); // 如果ucLed[i]为1, 则将temp的第i位置1
    }

    // 仅当当前状态与之前状态不同的时候，才更新显示
    if (temp != temp_old)
    {
        // 使用HAL库函数根据temp的值设置对应引脚状态 (假设高电平点亮)
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, (temp & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 0 (PB12)
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, (temp & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 1 (PB13)
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, (temp & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 2 (PB14)
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, (temp & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 3 (PB15)
        HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, (temp & 0x10) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 4 (PD8)
        HAL_GPIO_WritePin(LED6_GPIO_Port, LED6_Pin, (temp & 0x20) ? GPIO_PIN_SET : GPIO_PIN_RESET); // LED 5 (PD9)

        temp_old = temp;                // 更新记录的旧状态
    }
}


//led处理函数
void led_task(void)
{
    // 检查是否为WebSocket手动控制模式
    extern uint8_t websocket_get_led_control_mode(void);

    if (websocket_get_led_control_mode() == 1)
    {
        // 手动控制模式：只更新显示，不执行特效
        led_disp(ucLed);
        return;
    }

    // 自动模式：可以执行呼吸灯、流水灯等特效
//	wave_led();
    led_disp(ucLed);                    // 调用led_disp函数更新LED状态
}




void breath_led(void)
{
	/*============================呼吸灯========================*/
	
	/*
		如果pwmMax过大，频率过低
		那么就会有闪烁感
	因为呼吸之间跨度太大
	*/
	static uint32_t breathCounter = 0;		//呼吸计时器
	static uint8_t pwmCounter = 0;			//软件PWM计数器 LED公用
	static uint8_t brightness = 0;			//当前计算出的LED的亮度值
	static const uint16_t breathPeriod = 2000;	//呼吸周期
	static const uint8_t pwmMax = 10;		//PWM的精读
	
	
	//更新呼吸计时器，每次调用函数时+1 达到周期后归零
	//这个计数器相当于呼吸效果的时间轴
	breathCounter  = (breathCounter + 1) % breathPeriod;
	
	//核心：计算当前时刻的亮度值
	//使用正弦函数sinf来模拟平滑的亮度变化
	
	brightness = (uint8_t)((sinf((2.0f * 3.14159f * breathCounter) / breathPeriod) +1.0f) * pwmMax / 2.0f);


	
	//更新PWM计数器，快速计数
	pwmCounter = (pwmCounter + 1) % pwmMax;
	
	ucLed[0] = (pwmCounter < brightness) ? 1 : 0;
	
	

}


void wave_led(void)
{
	// 呼吸流水灯相关变量
    static uint32_t breathCounter = 0;      // 全局呼吸计时器
    static uint8_t pwmCounter = 0;          // 软件PWM计数器 (所有LED共用)
    static const uint16_t breathPeriod = 4000; // 整个流水呼吸效果的周期 (调慢一点，看得清楚)
    static const uint8_t pwmMax = 10;       // PWM精度 (同上)
    // 关键：相位差！决定了相邻LED呼吸节奏的错开程度
    // π / 3 意味着一个完整周期(2π)内能容纳 6 个LED (2π / (π/3) = 6)
    // 每个LED比前一个晚 π/3 的相位开始呼吸
    static const float phaseShift = 3.14159f / 3.0f;

    // 更新全局呼吸计时器
    breathCounter = (breathCounter + 1) % breathPeriod;

    // 更新PWM计数器
    pwmCounter = (pwmCounter + 1) % pwmMax;

    // 循环为每个LED计算独立的亮度并设置状态
    for(uint8_t i = 0; i < 6; i++) // 遍历所有6个LED
    {
        // 计算当前LED的相位角 (angle)
        // (2.0f * 3.14159f * breathCounter) / breathPeriod 是基础角度，随时间变化
        // - i * phaseShift 是为第 i 个LED引入的相位延迟
        float angle = (2.0f * 3.14159f * breathCounter) / breathPeriod - i * phaseShift;

        // 计算原始的正弦值 (-1.0 到 1.0)
        float sinValue = sinf(angle);

        // 增强对比度并调整曲线 (让亮灭更分明，全亮时间更短)
        // powf(x, 0.5f) 相当于 sqrt(x)，对于正数，它会把小的数拉高，大的数相对压低一点 (但这里主要用在 >0 的部分)
        // 对于负数，我们取绝对值再开根号，然后加回负号，保持形状对称
        // 这使得亮度从暗到亮的过程变快，从亮到暗的过程也变快，中间亮的时间缩短
        float enhancedValue = sinValue > 0 ? powf(sinValue, 0.5f) : -powf(-sinValue, 0.5f);

        // 进一步压缩亮度曲线，使得只有在接近峰值时才真正达到高亮度
        // 如果增强后的值大于0.7 (接近峰值)，则保持不变；否则，乘以0.6，让它更暗
        // 目的是让“光波“显得更窄，流动感更强
        enhancedValue = enhancedValue > 0.7f ? enhancedValue : enhancedValue * 0.6f;

        // 将处理后的 enhancedValue (-1 到 1 之间) 映射到 0 到 pwmMax 的亮度值
        uint8_t brightness = (uint8_t)((enhancedValue + 1.0f) * pwmMax / 2.0f);

        // 根据计算出的该LED的亮度，使用PWM逻辑设置其状态
        ucLed[i] = (pwmCounter < brightness) ? 1 : 0;
    }

}





























