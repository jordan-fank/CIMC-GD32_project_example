#include "scheduler.h"


uint8_t task_num;

//调度器结构体
typedef struct {
    void (*task_func)(void);
    uint32_t rate_ms;
    uint32_t last_run;
} task_t;



//构建任务列表
static task_t scheduler_task[] =
{
     {led_task,  1,  0}   // 定义一个任务，任务函数为 led_task，执行周期为 1 毫秒，初始上次运行时间为 0
    ,{adc_task,  100,  0}   // 定义一个任务，任务函数为 adc_task，执行周期为 1 毫秒，初始上次运行时间为 0		 
    ,{btn_task,  5,  0}   // 定义一个任务，任务函数为 btn_task，执行周期为 5 毫秒，初始上次运行时间为 0
    ,{uart_task, 5,  0}   // 定义一个任务，任务函数为 uart_task，执行周期为 5 毫秒，初始上次运行时间为 0
	,{websocket_task, 5,  0} 
	,{oled_task, 50,  0}	//如果使用u8g2就关闭这个
//	,{u8g2_task,50,0}
//	,{wououi_task,1,0}

};


//初始化任务数量
void scheduler_init(void)
{
	task_num = sizeof(scheduler_task) / sizeof(task_t);
}


//调度器
void scheduler_run(void)
{
    for (uint8_t i = 0; i < task_num; i++)
    {
        uint32_t now_time = HAL_GetTick();

        if (now_time >= scheduler_task[i].rate_ms + scheduler_task[i].last_run)
        {
            scheduler_task[i].last_run = now_time;


            scheduler_task[i].task_func();
        }
    }
}


