#include "btn_app.h"

/*===========================按键处理外部信息的变量================*/

//频率步进值
#define FREQ_STEP 100		//每次增加或减少100HZ

//波形幅度调节步进值
#define AMP_STEP 100		//每次增加或减少100mv

//波形类型设置--初始设置为正弦波
static dac_waveform_t current_wave_type = WAVEFORM_SINE;

//频率大小的限制
#define MIN_FREQUENCY 100		//最小频率为100
#define MAX_FREQUENCY 50000		//最大频率为50KHZ

//波形幅度限制
#define MIN_AMPLITUDE 100		//最小峰值幅度
#define MAX_AMPLITUDE 1650		//最大峰值幅度


//频率
uint32_t new_freq;
uint32_t current_freq; 

//幅度
uint16_t current_amp;
uint16_t new_amp;

//串口发送标志位---控制串口打印波形
uint8_t uart_send_flag = 0;






/*=============================按键库准备配置=====================*/
/* 按键ID定义 */
typedef enum
{
    USER_BUTTON_0 = 0,
    USER_BUTTON_1,
    USER_BUTTON_2,
    USER_BUTTON_3,
    USER_BUTTON_4,
    USER_BUTTON_5,
    USER_BUTTON_6,
    USER_BUTTON_7,
    USER_BUTTON_8,
    USER_BUTTON_9,
    USER_BUTTON_MAX,

    USER_BUTTON_COMBO_0 = 0x100, // 组合按键0
    USER_BUTTON_COMBO_1,         // 组合按键1
    USER_BUTTON_COMBO_2,         // 组合按键2
    USER_BUTTON_COMBO_MAX,
} user_button_t;


/* 按键参数配置 */
const ebtn_btn_param_t defaul_ebtn_param = EBTN_PARAMS_INIT(
    20,     // time_debounce: 按下稳定 20ms
    20,     // time_debounce_release: 释放稳定 20ms
    50,     // time_click_pressed_min: 最短单击按下 50ms
    500,    // time_click_pressed_max: 最长单击按下 500ms (超过则不算单击)
    300,    // time_click_multi_max: 多次单击最大间隔 300ms (两次点击间隔超过则重新计数)
    500,    // time_keepalive_period: 长按事件周期 500ms (按下超过 500ms 后，每 500ms 触发一次)
    5       // max_consecutive: 最多支持 5 连击
);


/* 按键数组 */
static ebtn_btn_t btns[] = {
    EBTN_BUTTON_INIT(USER_BUTTON_0, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_1, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_2, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_3, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_4, &defaul_ebtn_param),
    EBTN_BUTTON_INIT(USER_BUTTON_5, &defaul_ebtn_param),
};

/* 组合按键数组 */
static ebtn_btn_combo_t btns_combo[] = {
    EBTN_BUTTON_COMBO_INIT(USER_BUTTON_COMBO_0, &defaul_ebtn_param),  // 组合按键0: 按键0 + 按键1
    EBTN_BUTTON_COMBO_INIT(USER_BUTTON_COMBO_1, &defaul_ebtn_param),  // 组合按键1: 按键0 + 按键2
    EBTN_BUTTON_COMBO_INIT(USER_BUTTON_COMBO_2, &defaul_ebtn_param),  // 组合按键2: 按键0 + 按键3
};


/* 获取按键状态函数 */
uint8_t prv_btn_get_state(struct ebtn_btn *btn)
{
    switch (btn->key_id)
    {
    case USER_BUTTON_0: return !HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin);
    case USER_BUTTON_1: return !HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin);
    case USER_BUTTON_2: return !HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin);
    case USER_BUTTON_3: return !HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin);
    case USER_BUTTON_4: return !HAL_GPIO_ReadPin(KEY5_GPIO_Port, KEY5_Pin);
    case USER_BUTTON_5: return !HAL_GPIO_ReadPin(KEY6_GPIO_Port, KEY6_Pin);
    default: return 0;
    }
}





/*=====================================按键处理=================================*/

/* 按键事件处理函数 */
void prv_btn_event(struct ebtn_btn *btn, ebtn_evt_t evt)
{

/*=======================================================单多击按键处理部分====================================*/

    switch (evt)
    {
		
/*=======================================================测试单击和多击打====================================*/

//	uint16_t click_cnt = ebtn_click_get_count(btn);	
//    case EBTN_EVT_ONCLICK:

//        // 单击、双击按键处理
//        switch (btn->key_id)
//        {
//        case USER_BUTTON_0:
//            if (click_cnt == 1)
//                ucLed[0] = 1;  // 单击点亮LED
//            else if (click_cnt == 2)
//                ucLed[0] = 0;  // 双击熄灭LED
//            break;

//        case USER_BUTTON_1:
//            if (click_cnt == 1)
//                ucLed[1] = 1;
//            else if (click_cnt == 2)
//                ucLed[1] = 0;
//            break;
//        case USER_BUTTON_2:
//            break;
//        case USER_BUTTON_3:
//            break;
//        case USER_BUTTON_4:
//            break;
//        case USER_BUTTON_5:
//            break;

//        default:
//            break;
//        }

	//单击事件
/*=============================================ADDA波形处理按键部分==========================================*/

//		case EBTN_EVT_ONCLICK:
//		//按键1单击一次----切换串口发送标志位
//		if((btn->key_id == USER_BUTTON_0) && (ebtn_click_get_count(btn) == 1))
//		{
//			ucLed[0] ^= 1;

//			uart_send_flag ^= 1;;
//			wave_analysis_flag = 1;
//		}
//		
//		//按键2单击一次----切换波形
//		if((btn->key_id == USER_BUTTON_1) && (ebtn_click_get_count(btn) == 1))
//		{
//			ucLed[1] ^= 1;
//			
//			switch(current_wave_type)
//			{
//				case WAVEFORM_SINE:
//					current_wave_type = WAVEFORM_SQUARE;
//				break;
//				
//				case WAVEFORM_SQUARE:
//					current_wave_type = WAVEFORM_TRIANGLE;
//				break;
//				
//				case WAVEFORM_TRIANGLE:
//					current_wave_type = WAVEFORM_SINE;
//				break;
//				
//				default:
//					current_wave_type = WAVEFORM_SINE;
//				break;
//			}
//			
//			//设置新的波形类型
//			dac_app_set_waveform(current_wave_type);
//		}

//		//按键3单击1次----增加频率
//		if((btn->key_id == USER_BUTTON_2) && (ebtn_click_get_count(btn) == 1))
//		{
//			ucLed[2] ^= 1;
//			
//			//获取当期那频率，然后增加FREQ_STEP
//			current_freq = dac_app_get_update_frequency() / WAVEFORM_SAMPLES;
//			new_freq = current_freq + FREQ_STEP;
//			
//			//设置最大频率
//			if(new_freq > MAX_FREQUENCY)
//				new_freq = MAX_FREQUENCY;
//			
//			//设置新频率
//			dac_app_set_frequency(new_freq);
//		}

//		//按键4单击1次----减少频率
//		if ((btn->key_id == USER_BUTTON_3) && (ebtn_click_get_count(btn) == 1))
//		{
//			// 按键3减少频率
//			ucLed[3] ^= 1;

//			// 获取当前频率，然后减少FREQ_STEP
//			current_freq = dac_app_get_update_frequency() / WAVEFORM_SAMPLES;
//			new_freq = (current_freq > FREQ_STEP) ? (current_freq - FREQ_STEP) : MIN_FREQUENCY;

//			// 限制最小频率
//			if (new_freq < MIN_FREQUENCY)
//				new_freq = MIN_FREQUENCY;

//			// 设置新频率
//			dac_app_set_frequency(new_freq);
//		}
//		
//		//按键5单击一次 增加峰值幅度
//		if ((btn->key_id == USER_BUTTON_4) && (ebtn_click_get_count(btn) == 1))
//		{
//			// 按键4增加峰值幅度
//			ucLed[4] ^= 1;

//			// 获取当前峰值幅度并增加AMP_STEP
//			current_amp = dac_app_get_amplitude();
//			new_amp = current_amp + AMP_STEP;

//			// 限制最大幅度
//			if (new_amp > MAX_AMPLITUDE)
//				new_amp = MAX_AMPLITUDE;

//			// 设置新幅度
//			dac_app_set_amplitude(new_amp);
//		}

//		//按键6单击一次  减少幅度
//		if ((btn->key_id == USER_BUTTON_5) && (ebtn_click_get_count(btn) == 1))
//		{
//			// 按键5减少峰值幅度
//			ucLed[5] ^= 1;

//			// 获取当前峰值幅度并减少AMP_STEP
//			current_amp = dac_app_get_amplitude();
//			new_amp = (current_amp > AMP_STEP) ? (current_amp - AMP_STEP) : MIN_AMPLITUDE;

//			// 限制最小幅度
//			if (new_amp < MIN_AMPLITUDE)
//				new_amp = MIN_AMPLITUDE;

//			// 设置新幅度
//			dac_app_set_amplitude(new_amp);
//		}
//		
//		break;

		
/*=============================================WouoUI部分处理按键部分==========================================*/
		
		case EBTN_EVT_ONCLICK:
			//按键1单击一次----对应菜单'上'的操作
		if((btn->key_id == USER_BUTTON_0) && (ebtn_click_get_count(btn) == 1))
		{
			ucLed[0] ^= 1;

			 // 发送"上"消息给 WouoUI
			WOUOUI_MSG_QUE_SEND(msg_up);
		}
		
		//按键2单击一次----对应菜单'下'的操作
		if((btn->key_id == USER_BUTTON_1) && (ebtn_click_get_count(btn) == 1))
		{
			ucLed[1] ^= 1;
			WOUOUI_MSG_QUE_SEND(msg_down);
		}

		//按键3单击1次----对应菜单‘左’的操作
		if((btn->key_id == USER_BUTTON_2) && (ebtn_click_get_count(btn) == 1))
		{
			ucLed[2] ^= 1;
			 WOUOUI_MSG_QUE_SEND(msg_left);
		}

		//按键4单击1次----对应菜单‘右’的操作
		if ((btn->key_id == USER_BUTTON_3) && (ebtn_click_get_count(btn) == 1))
		{
			// 按键3减少频率
			ucLed[3] ^= 1;
			WOUOUI_MSG_QUE_SEND(msg_right);
		}
		
		//按键5单击一次----对应菜单‘返回’的操作
		if ((btn->key_id == USER_BUTTON_4) && (ebtn_click_get_count(btn) == 1))
		{
			// 按键4增加峰值幅度
			ucLed[4] ^= 1;

			WOUOUI_MSG_QUE_SEND(msg_return);
		}

		//按键6单击一次----对应菜单‘确认’的操作
		if ((btn->key_id == USER_BUTTON_5) && (ebtn_click_get_count(btn) == 1))
		{
			// 按键5减少峰值幅度
			ucLed[5] ^= 1;
			WOUOUI_MSG_QUE_SEND(msg_click);
		}

		break;
/*=============================================长按键处理部分==================================================*/

	//长按事件
    case EBTN_EVT_KEEPALIVE:
        // 长按处理：根据 keepalive_cnt 执行操作
        if (btn->key_id == USER_BUTTON_0 && btn->keepalive_cnt >= 3)
        {
//            ucLed[0] = 1;  // 长按3次周期点亮LED
        }
        break;

		
		
		
		
/*=================================组合按键处理部分===========================*/
    
	//组合按键事件
	case EBTN_EVT_ONRELEASE:
        // 组合按键事件处理
        if (btn->key_id == USER_BUTTON_COMBO_0)
        {
			 ucLed[3] ^= 1;
            // 按键0 + 按键1 复制 ucLed
//            memcpy(ucLed_copy, ucLed, sizeof(ucLed));
           
        }
        else if (btn->key_id == USER_BUTTON_COMBO_1)
        {
			ucLed[4] ^= 1;
            // 按键0 + 按键2 粘贴 ucLed
//            memcpy(ucLed, ucLed_copy, sizeof(ucLed));
           
        }
        else if (btn->key_id == USER_BUTTON_COMBO_2)
        {
			ucLed[5] ^= 1;
            // 按键0 + 按键3 剪切 ucLed
//            memcpy(ucLed_copy, ucLed, sizeof(ucLed));
//            memset(ucLed, 0, sizeof(ucLed));  // 清空 ucLed
        
        }
        break;



    default:
        break;
	
    }
	

}




/*=====================================按键初始化===========================*/

/* 初始化按键驱动 */
void app_ebtn_init(void)
{
    // 初始化按键库，并传入组合按键数组
    ebtn_init(btns, EBTN_ARRAY_SIZE(btns), btns_combo, EBTN_ARRAY_SIZE(btns_combo), prv_btn_get_state, prv_btn_event);
	
	ebtn_set_config(EBTN_CFG_COMBO_PRIORITY);

    // 配置组合按键
    ebtn_combo_btn_add_btn(&btns_combo[0], USER_BUTTON_0);  // 组合按键0: 按键0 + 按键1
    ebtn_combo_btn_add_btn(&btns_combo[0], USER_BUTTON_1);
    
    ebtn_combo_btn_add_btn(&btns_combo[1], USER_BUTTON_0);  // 组合按键1: 按键0 + 按键2
    ebtn_combo_btn_add_btn(&btns_combo[1], USER_BUTTON_2);

    ebtn_combo_btn_add_btn(&btns_combo[2], USER_BUTTON_0);  // 组合按键2: 按键0 + 按键3
    ebtn_combo_btn_add_btn(&btns_combo[2], USER_BUTTON_3);
	
	//专门为按键配置的定时器强制触发，以免被堵塞
	HAL_TIM_Base_Start_IT(&htim13);
}

//定时器13回调函数，每10Ms强制执行一次定时器处理，以免堵塞
void  HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
		if(htim == (&htim13))
		{
			ebtn_process(HAL_GetTick());

		}
}


void btn_task(void)
{
	
}

