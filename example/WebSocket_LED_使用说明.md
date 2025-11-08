# WebSocket LED控制系统使用说明

## 功能概述
本系统实现了通过网页远程控制6个LED灯的功能，使用WebSocket协议进行实时通信。

## 系统架构
```
网页界面 ←→ WebSocket服务器 ←→ 4G模块 ←→ 单片机 ←→ LED硬件
```

## 协议格式
### 控制指令
```
LED:X,Y
```
- X: LED编号 (1-6)
- Y: LED状态 (0=关闭, 1=开启)

示例：
- `LED:1,1` → 开启LED1
- `LED:1,0` → 关闭LED1
- `LED:3,1` → 开启LED3

### 响应格式
```
LED:X,Y,OK
```
- X: LED编号
- Y: 执行后的状态
- OK: 执行结果确认

## 使用步骤

### 1. 服务器端（已实现）
- WebSocket服务器运行在端口47315
- HTTP服务器运行在端口8080
- 支持FRP内网穿透：`hk-4.lcf.im:47315`

### 2. 单片机端（已实现）
- 串口6接收WebSocket数据
- `websocket_parse_led_command()` 解析控制命令
- 直接操作`ucLed[6]`数组控制LED状态
- 通过串口1输出调试信息

### 3. 网页端（已实现）
- 访问 `http://localhost:8080/debug.html`
- 或通过公网 `http://www.yanjin.xyz` 后点击调试模式
- 点击LED控制按钮发送指令

## 代码结构

### 前端文件
- `debug.html` - LED控制界面
- `js/debug.js` - WebSocket通信逻辑

### 后端文件
- `websocket-server.js` - WebSocket服务器
- `http-server.js` - HTTP服务器

### 单片机文件
- `4g_web.c` - WebSocket数据处理和LED协议解析
- `4g_web.h` - 相关函数声明
- `led_app.c` - LED硬件控制
- `uart_app.c` - 串口通信框架

## 调试信息
通过串口1可以查看详细的调试信息：
- `[WebSocket] 收到数据: LED:1,1`
- `[WebSocket] LED1: 开启`
- `[WebSocket] 发送响应: LED:1,1,OK`

## 测试方法
1. 启动WebSocket服务器：`npm start`
2. 访问调试模式网页
3. 点击LED控制按钮
4. 观察单片机串口1输出
5. 验证LED硬件状态变化

## 错误处理
- LED编号范围：1-6
- LED状态值：0或1
- 命令格式：必须包含逗号分隔符
- 错误信息通过串口1输出

## 扩展功能
系统采用模块化设计，可轻松扩展：
- 添加更多控制命令
- 支持PWM调光
- 添加传感器数据上传
- 实现批量控制功能