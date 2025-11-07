# 🎉 WebSocket终端管理系统 - 功能完全恢复

## ✅ 系统状态

### **原有功能已100%恢复** ✅
- ✅ `npm start` 正常工作，启动统一服务器
- ✅ HTTP服务：http://localhost:8080
- ✅ WebSocket服务：ws://localhost:8080
- ✅ WebSocket心跳包和消息转发正常
- ✅ 前端界面完全正常
- ✅ 4G模块连接支持

### **新增公网访问功能** ✅
- ✅ 统一服务器（HTTP + WebSocket在同一端口）
- ✅ HTTP静态文件服务
- ✅ WebSocket实时通信
- ✅ 前端自动域名检测和配置
- ✅ 支持多客户端同时访问

---

## 🚀 立即使用

### 启动服务器
```bash
cd C:\Users\HP\Desktop\CIMC_xi\web\backend
npm start
```

### 本地访问
- **网页**: http://localhost:8080
- **WebSocket**: ws://localhost:8080

### 配置公网访问

1. **配置FRP HTTP映射**:
   ```ini
   [web_terminal]
   type = http
   local_ip = 127.0.0.1
   local_port = 8080
   custom_domains = www.yanjin.xyz
   ```

2. **配置完成后，公网访问**:
   - **网页**: http://www.yanjin.xyz
   - **自动配置**: 页面会自动检测域名并填充连接参数
   - **WebSocket**: ws://www.yanjin.xyz

---

## 📁 项目结构

```
web/
├── backend/                    # 后端服务器
│   ├── server-combined.js      # 统一服务器（HTTP + WebSocket）
│   ├── server.js               # 原始WebSocket服务器
│   ├── test-websocket.js       # 测试脚本
│   └── package.json            # 依赖配置

└── frontend/                   # 前端页面
    ├── index.html              # 主页面
    ├── css/
    │   └── style.css           # 样式文件
    └── js/
        └── app.js              # 业务逻辑
```

---

## 📡 使用流程

### 对服务器端（你）:
1. **启动服务器**: `npm start`
2. **保持FRP运行**: 确保HTTP映射正常
3. **正常使用**: 4G模块和网页都能正常工作

### 对访问者（其他人）:
1. **浏览器访问**: http://www.yanjin.xyz
2. **自动配置**: 页面自动检测域名，自动填充连接参数
3. **点击连接**: 点击"连接服务器"按钮
4. **实时通信**: 发送和接收消息

---

## 🔧 配置说明

### 后端配置

#### WebSocket服务器 (`server.js`)

```javascript
const WS_PORT = 8080;                  // WebSocket端口
const HEARTBEAT_INTERVAL = 30000;      // 心跳检测间隔（毫秒）
```

#### HTTP服务器 (`http-server.js`)

```javascript
const HTTP_PORT = 3000;                // HTTP服务端口
const FRONTEND_DIR = '../frontend';    // 前端文件目录
```

### 前端配置

前端页面支持**自定义服务器地址和端口**，配置会自动保存到浏览器的本地存储。

---

## 💡 核心功能

### 1. WebSocket 数据转发

- ✅ 接收客户端消息
- ✅ 自动转发给所有其他已连接的客户端
- ✅ 支持多客户端并发连接
- ✅ 心跳检测维持连接活性

**转发逻辑**：

```javascript
// 当前实现：转发给所有其他客户端（不包括发送者）
broadcastMessage(sender, data);

// 如需转发给所有客户端（包括发送者），修改为：
broadcastToAll(data);
```

### 2. 前端功能

- ✅ 可配置服务器地址和端口
- ✅ 实时连接状态显示
- ✅ 消息发送和接收
- ✅ 消息日志记录（带时间戳和类型标识）
- ✅ 统计信息（发送/接收计数、连接时长）
- ✅ 配置自动保存到本地存储

---

## 📡 WebSocket 协议

### 连接

```
ws://服务器地址:端口
```

### 消息格式

**系统消息**（服务器发送）：

```json
{
  "type": "system",
  "message": "连接成功",
  "timestamp": 1699999999999,
  "clientCount": 3
}
```

**用户消息**（客户端发送）：

可以是任意格式（文本、JSON等），服务器会原样转发。

---

## 🛠️ 技术栈

### 后端

- **Node.js**: JavaScript运行时
- **ws**: WebSocket库（高性能、轻量级）
- **http**: 内置HTTP模块（静态文件服务）
- **fs/path**: 文件系统操作

### 前端

- **HTML5**: 页面结构
- **CSS3**: 样式（渐变、动画、响应式）
- **原生 JavaScript**: 业务逻辑（ES6+）
- **WebSocket API**: 浏览器原生WebSocket

---

## 📊 使用场景

1. **嵌入式终端监控**
   - STM32/GD32等单片机通过串口桥接WebSocket
   - 实时上传传感器数据
   - 多客户端同时查看

2. **实时数据同步**
   - 多个终端设备间数据共享
   - 服务器自动转发，无需额外处理

3. **远程调试**
   - 通过浏览器实时查看设备日志
   - 发送调试命令

4. **消息广播系统**
   - 一对多实时消息推送

---

## 🔒 安全注意事项

### 当前实现

- ✅ 路径遍历攻击防护（HTTP服务器）
- ✅ XSS防护（前端HTML转义）
- ✅ 心跳检测（防止僵尸连接）

### 生产环境建议

1. **使用 WSS（WebSocket over TLS）**
   ```javascript
   const wss = new WebSocket.Server({ server: httpsServer });
   ```

2. **添加身份验证**
   - Token验证
   - 连接握手验证

3. **限流和防护**
   - 连接数限制
   - 消息大小限制
   - 频率限制

4. **消息验证**
   - 数据格式校验
   - 内容过滤

---

## 🧪 测试方法

### 测试WebSocket服务器

使用在线WebSocket测试工具或命令行：

```bash
# 使用wscat（需先安装: npm install -g wscat）
wscat -c ws://localhost:8080

# 发送消息
> Hello, World!

# 观察其他客户端是否收到
```

### 测试多客户端转发

1. 打开多个浏览器标签页访问 `http://localhost:3000`
2. 分别连接到WebSocket服务器
3. 在任意客户端发送消息
4. 观察其他客户端是否实时接收

---

## 📝 开发扩展

### 添加新的API端点

在 `server.js` 中处理特定消息类型：

```javascript
ws.on('message', (data) => {
    try {
        const message = JSON.parse(data);

        // 根据消息类型分发
        switch (message.type) {
            case 'command':
                handleCommand(ws, message);
                break;
            case 'data':
                handleData(ws, message);
                break;
            default:
                broadcastMessage(ws, data);
        }
    } catch (error) {
        // 不是JSON，原样转发
        broadcastMessage(ws, data);
    }
});
```

### 前端添加新功能

编辑 `frontend/js/app.js`，在 `TerminalManager` 类中添加方法：

```javascript
class TerminalManager {
    // ... 现有代码 ...

    /**
     * 发送命令到服务器
     */
    sendCommand(command, params) {
        const message = JSON.stringify({
            type: 'command',
            command: command,
            params: params,
            timestamp: Date.now()
        });
        this.ws.send(message);
    }
}
```

---

## 🐛 常见问题

### Q1: 无法连接到WebSocket服务器

**检查项**：
- WebSocket服务器是否已启动
- 端口是否被占用（默认8080）
- 防火墙是否阻止连接
- 浏览器控制台是否有错误信息

### Q2: 前端页面无法访问

**检查项**：
- HTTP服务器是否已启动
- 端口是否正确（默认3000）
- 浏览器地址是否正确（`http://localhost:3000`）

### Q3: 消息无法转发

**检查项**：
- 查看服务器控制台是否有错误日志
- 确认多个客户端都已成功连接
- 检查浏览器开发者工具的WebSocket帧

---

## 📞 支持

遇到问题或有建议？请检查：

1. 服务器控制台日志
2. 浏览器开发者工具（F12 -> Console / Network）
3. 本文档的常见问题部分

---

## 📄 许可证

ISC License

---

## 🎯 下一步开发计划

- [ ] 添加用户认证
- [ ] 支持房间/频道功能
- [ ] 消息持久化（数据库）
- [ ] 文件传输支持
- [ ] 更丰富的前端UI组件
- [ ] RESTful API支持
- [ ] Docker部署支持

---

**版本**: 1.0.0
**最后更新**: 2025-11-06
