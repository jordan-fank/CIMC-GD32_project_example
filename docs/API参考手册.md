# GD32项目框架API参考手册

## 1. 任务调度器 API (scheduler)

### 1.1 数据结构
```c
typedef struct {
    void (*task_func)(void);  // 任务函数指针
    uint32_t rate_ms;         // 执行周期(毫秒)
    uint32_t last_run;        // 上次执行时间
} task_t;
```

### 1.2 函数接口

#### `scheduler_init(void)`
**功能**: 初始化任务调度器
**参数**: 无
**返回值**: 无
**使用示例**:
```c
scheduler_init();  // 在main()函数中调用
```

#### `scheduler_run(void)`
**功能**: 执行任务调度器
**参数**: 无
**返回值**: 无
**使用示例**:
```c
while (1) {
    scheduler_run();  // 在主循环中调用
}
```

### 1.3 添加新任务
```c
// 在scheduler.c中的scheduler_task数组添加
{your_task_function, 100, 0}  // 100ms执行周期
```

---

## 2. LED控制 API (led_app)

### 2.1 数据结构
```c
extern uint8_t ucLed[6];  // LED状态数组，0=关闭，1=开启
```

### 2.2 函数接口

#### `led_init(void)`
**功能**: 初始化LED系统
**参数**: 无
**返回值**: 无

#### `led_task(void)`
**功能**: LED任务函数，在调度器中调用
**参数**: 无
**返回值**: 无

#### `led_disp(uint8_t *ucLed)`
**功能**: 显示LED状态
**参数**:
- `ucLed`: LED状态数组指针
**返回值**: 无

#### `websocket_get_led_control_mode(void)`
**功能**: 获取LED控制模式
**参数**: 无
**返回值**: `uint8_t` - 1=WebSocket控制模式，0=本地控制模式

---

## 3. 按键处理 API (btn_app)

### 3.1 数据结构
```c
typedef enum {
    BTN_EVENT_PRESS,     // 按下事件
    BTN_EVENT_RELEASE,   // 释放事件
    BTN_EVENT_LONG_PRESS, // 长按事件
    BTN_EVENT_DOUBLE_CLICK // 双击事件
} btn_event_t;

typedef struct {
    uint8_t pin;              // 按键引脚
    btn_event_t event;        // 事件类型
    uint32_t timestamp;       // 事件时间戳
} btn_data_t;
```

### 3.2 函数接口

#### `btn_init(void)`
**功能**: 初始化按键系统
**参数**: 无
**返回值**: 无

#### `btn_task(void)`
**功能**: 按键扫描任务
**参数**: 无
**返回值**: 无

#### `btn_register_callback(void (*callback)(btn_data_t))`
**功能**: 注册按键回调函数
**参数**:
- `callback`: 回调函数指针
**返回值**: 无

---

## 4. 串口通信 API (uart_app)

### 4.1 数据结构
```c
typedef struct {
    uint8_t *buffer;     // 缓冲区指针
    uint16_t size;       // 缓冲区大小
    uint16_t head;       // 头指针
    uint16_t tail;       // 尾指针
} ringbuffer_t;
```

### 4.2 函数接口

#### `uart_init(void)`
**功能**: 初始化串口
**参数**: 无
**返回值**: 无

#### `uart_task(void)`
**功能**: 串口任务函数
**参数**: 无
**返回值**: 无

#### `uart_send_string(const char *str)`
**功能**: 发送字符串
**参数**:
- `str`: 要发送的字符串
**返回值**: 无

#### `uart_send_data(uint8_t *data, uint16_t len)`
**功能**: 发送数据
**参数**:
- `data`: 数据指针
- `len`: 数据长度
**返回值**: 无

#### `uart_get_received_data(uint8_t *buffer, uint16_t *len)`
**功能**: 获取接收到的数据
**参数**:
- `buffer`: 接收缓冲区
- `len`: 接收长度指针
**返回值**: `uint8_t` - 1=有数据，0=无数据

---

## 5. ADC采集 API (adc_app)

### 5.1 函数接口

#### `adc_init(void)`
**功能**: 初始化ADC
**参数**: 无
**返回值**: 无

#### `adc_task(void)`
**功能**: ADC任务函数
**参数**: 无
**返回值**: 无

#### `adc_get_value(uint8_t channel)`
**功能**: 获取ADC值
**参数**:
- `channel`: ADC通道号
**返回值**: `uint16_t` - ADC转换值

#### `adc_get_voltage(uint8_t channel)`
**功能**: 获取电压值
**参数**:
- `channel`: ADC通道号
**返回值**: `float` - 电压值(V)

---

## 6. DAC输出 API (dac_app)

### 6.1 函数接口

#### `dac_init(void)`
**功能**: 初始化DAC
**参数**: 无
**返回值**: 无

#### `dac_task(void)`
**功能**: DAC任务函数
**参数**: 无
**返回值**: 无

#### `dac_set_value(uint8_t channel, uint16_t value)`
**功能**: 设置DAC输出值
**参数**:
- `channel`: DAC通道号
- `value`: DAC值 (0-4095)
**返回值**: 无

#### `dac_set_voltage(uint8_t channel, float voltage)`
**功能**: 设置DAC输出电压
**参数**:
- `channel`: DAC通道号
- `voltage`: 电压值 (0-3.3V)
**返回值**: 无

---

## 7. Flash存储 API (flash_app)

### 7.1 函数接口

#### `flash_init(void)`
**功能**: 初始化Flash
**参数**: 无
**返回值**: 无

#### `flash_erase_sector(uint32_t sector)`
**功能**: 擦除Flash扇区
**参数**:
- `sector`: 扇区号
**返回值**: `uint8_t` - 1=成功，0=失败

#### `flash_write(uint32_t address, uint8_t *data, uint16_t len)`
**功能**: 写入Flash
**参数**:
- `address`: 写入地址
- `data`: 数据指针
- `len`: 数据长度
**返回值**: `uint8_t` - 1=成功，0=失败

#### `flash_read(uint32_t address, uint8_t *data, uint16_t len)`
**功能**: 读取Flash
**参数**:
- `address`: 读取地址
- `data`: 数据指针
- `len`: 数据长度
**返回值**: `uint8_t` - 1=成功，0=失败

---

## 8. Shell系统 API (shell_app)

### 8.1 数据结构
```c
typedef struct {
    char *name;           // 命令名称
    void (*func)(char *args); // 命令函数
    char *help;           // 帮助信息
} shell_cmd_t;
```

### 8.2 函数接口

#### `shell_init(void)`
**功能**: 初始化Shell系统
**参数**: 无
**返回值**: 无

#### `shell_task(void)`
**功能**: Shell任务函数
**参数**: 无
**返回值**: 无

#### `shell_register_command(shell_cmd_t *cmd)`
**功能**: 注册Shell命令
**参数**:
- `cmd`: 命令结构体指针
**返回值**: 无

#### `shell_process(char *input)`
**功能**: 处理Shell输入
**参数**:
- `input`: 输入字符串
**返回值**: 无

---

## 9. WebSocket通信 API (4g_web)

### 9.1 函数接口

#### `websocket_init(void)`
**功能**: 初始化WebSocket
**参数**: 无
**返回值**: 无

#### `websocket_task(void)`
**功能**: WebSocket任务函数
**参数**: 无
**返回值**: 无

#### `websocket_send_data(uint8_t *data, uint16_t len)`
**功能**: 发送WebSocket数据
**参数**:
- `data`: 数据指针
- `len`: 数据长度
**返回值**: `uint8_t` - 1=成功，0=失败

#### `websocket_send_string(char *str)`
**功能**: 发送WebSocket字符串
**参数**:
- `str`: 字符串
**返回值**: `uint8_t` - 1=成功，0=失败

#### `websocket_parse_led_command(uint8_t *buffer, uint16_t length)`
**功能**: 解析LED控制命令
**参数**:
- `buffer`: 接收缓冲区
- `length`: 数据长度
**返回值**: 无

#### `websocket_send_led_response(uint8_t led_num, uint8_t state)`
**功能**: 发送LED控制响应
**参数**:
- `led_num`: LED编号
- `state`: LED状态
**返回值**: 无

---

## 10. OLED显示 API (oled_app)

### 10.1 函数接口

#### `oled_init(void)`
**功能**: 初始化OLED
**参数**: 无
**返回值**: 无

#### `oled_task(void)`
**功能**: OLED任务函数
**参数**: 无
**返回值**: 无

#### `oled_clear(void)`
**功能**: 清屏
**参数**: 无
**返回值**: 无

#### `oled_display_string(uint8_t x, uint8_t y, char *str)`
**功能**: 显示字符串
**参数**:
- `x`: X坐标
- `y`: Y坐标
- `str`: 字符串
**返回值**: 无

#### `oled_draw_pixel(uint8_t x, uint8_t y, uint8_t color)`
**功能**: 画像素点
**参数**:
- `x`: X坐标
- `y`: Y坐标
- `color`: 颜色 (0=黑色，1=白色)
**返回值**: 无

---

## 11. SD卡文件系统 API (sd_fatfs)

### 11.1 函数接口

#### `sd_init(void)`
**功能**: 初始化SD卡
**参数**: 无
**返回值**: `uint8_t` - 1=成功，0=失败

#### `sd_mount(void)`
**功能**: 挂载文件系统
**参数**: 无
**返回值**: `uint8_t` - 1=成功，0=失败

#### `sd_read_file(char *filename, uint8_t *buffer, uint32_t *size)`
**功能**: 读取文件
**参数**:
- `filename`: 文件名
- `buffer`: 读取缓冲区
- `size`: 文件大小指针
**返回值**: `uint8_t` - 1=成功，0=失败

#### `sd_write_file(char *filename, uint8_t *buffer, uint32_t size)`
**功能**: 写入文件
**参数**:
- `filename`: 文件名
- `buffer`: 写入缓冲区
- `size`: 数据大小
**返回值**: `uint8_t` - 1=成功，0=失败

---

## 12. 环形缓冲区 API (ringbuffer)

### 12.1 函数接口

#### `ringbuffer_init(ringbuffer_t *rb, uint8_t *buffer, uint16_t size)`
**功能**: 初始化环形缓冲区
**参数**:
- `rb`: 缓冲区结构体指针
- `buffer`: 缓冲区指针
- `size`: 缓冲区大小
**返回值**: 无

#### `ringbuffer_put(ringbuffer_t *rb, uint8_t data)`
**功能**: 写入数据
**参数**:
- `rb`: 缓冲区结构体指针
- `data`: 要写入的数据
**返回值**: `uint8_t` - 1=成功，0=缓冲区满

#### `ringbuffer_get(ringbuffer_t *rb, uint8_t *data)`
**功能**: 读取数据
**参数**:
- `rb`: 缓冲区结构体指针
- `data`: 读取数据指针
**返回值**: `uint8_t` - 1=成功，0=缓冲区空

#### `ringbuffer_available(ringbuffer_t *rb)`
**功能**: 获取可用数据长度
**参数**:
- `rb`: 缓冲区结构体指针
**返回值**: `uint16_t` - 可用数据长度

---

## 13. 宏定义和常量

### 13.1 错误码定义
```c
#define SUCCESS         1
#define ERROR           0
#define TIMEOUT         2
#define BUSY            3
#define INVALID_PARAM   4
```

### 13.2 LED编号定义
```c
#define LED1            0
#define LED2            1
#define LED3            2
#define LED4            3
#define LED5            4
#define LED6            5
```

### 13.3 按键引脚定义
```c
#define KEY1_PIN        GPIO_PIN_0
#define KEY1_PORT       GPIOA
#define KEY2_PIN        GPIO_PIN_1
#define KEY2_PORT       GPIOA
```

### 13.4 串口定义
```c
#define UART_DEBUG      USART1
#define UART_4G         USART2
#define UART_PC         USART3
```

---

## 14. 使用示例

### 14.1 完整的系统初始化示例
```c
int main(void) {
    // HAL库初始化
    HAL_Init();
    SystemClock_Config();

    // 外设初始化
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    MX_ADC1_Init();
    MX_DAC1_Init();

    // 应用模块初始化
    scheduler_init();
    led_init();
    btn_init();
    uart_init();
    adc_init();
    dac_init();
    flash_init();
    shell_init();
    websocket_init();
    oled_init();
    sd_init();

    // 主循环
    while (1) {
        scheduler_run();
    }
}
```

### 14.2 WebSocket LED控制示例
```c
// 在4g_web.c中的命令解析函数
void websocket_parse_led_command(uint8_t *buffer, uint16_t length) {
    if (strncmp((char*)buffer, "LED1_ON", 7) == 0) {
        websocket_led_control_mode = 1;
        ucLed[0] = 1;  // LED1开启
        led_disp(ucLed);
        websocket_send_led_response(1, 1);
    } else if (strncmp((char*)buffer, "LED1_OFF", 8) == 0) {
        websocket_led_control_mode = 1;
        ucLed[0] = 0;  // LED1关闭
        led_disp(ucLed);
        websocket_send_led_response(1, 0);
    }
}
```

### 14.3 Shell命令注册示例
```c
// 自定义命令函数
void cmd_led(char *args) {
    if (strstr(args, "on")) {
        ucLed[0] = 1;
        printf("LED1 ON\r\n");
    } else if (strstr(args, "off")) {
        ucLed[0] = 0;
        printf("LED1 OFF\r\n");
    }
}

// 注册命令
shell_cmd_t led_cmd = {
    .name = "led",
    .func = cmd_led,
    .help = "led [on|off] - Control LED1"
};

// 在初始化中注册
shell_register_command(&led_cmd);
```

---

## 15. 注意事项

1. **线程安全**: 所有API函数都是非线程安全的，只能在主线程中调用
2. **中断处理**: 避免在中断服务程序中调用这些API
3. **内存管理**: 使用静态分配，避免动态内存分配
4. **错误处理**: 所有函数都应检查返回值
5. **性能考虑**: 避免在任务中执行耗时操作

---

**版本**: v1.0
**更新时间**: 2025-11-08
**适用芯片**: GD32F470, STM32F429