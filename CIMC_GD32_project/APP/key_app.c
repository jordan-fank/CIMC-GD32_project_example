#include "key_app.h"

uint8_t key_val,key_old,key_down,key_up;

uint8_t key_read()
{
	uint8_t temp = 0;
	
	if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin) == GPIO_PIN_RESET) temp = 1; 
	if(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin) == GPIO_PIN_RESET) temp = 2; 
	if(HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin) == GPIO_PIN_RESET) temp = 3; 
	if(HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin) == GPIO_PIN_RESET) temp = 4; 
	if(HAL_GPIO_ReadPin(KEY5_GPIO_Port, KEY5_Pin) == GPIO_PIN_RESET) temp = 5; 
	if(HAL_GPIO_ReadPin(KEY6_GPIO_Port, KEY6_Pin) == GPIO_PIN_RESET) temp = 6; 
	
	return temp;
}


bool first_flag = 0;
bool second_flag = 0;
uint32_t now_time,last_time;

bool double_press = 0;


void key_task()
{
	key_val = key_read();
	key_down = key_val & (key_old ^ key_val);
	key_up = ~key_val & (key_old ^ key_val);
	key_old = key_val;
	


	switch(key_down)
	{

		
		case 1:
			
			
			break;
		
		case 2:
			break;
		
		case 3:
			break;
		
		case 4:
			break;
		
		case 5:
			break;
		
		case 6:
	
		break;
		
	}
	
	

	
}



