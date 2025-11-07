# Claude Relay Meter 代理服务器

## 🎯 解决的问题

- **原问题**: Claude Relay Meter 插件无法通过 API Key 获取 API ID，返回 403/404 错误
- **根本原因**: API 服务器地址不兼容，插件期望的统计端点不存在
- **解决方案**: 创建本地代理服务器，将插件请求转换为兼容的 API 调用

## 📁 文件说明

- **`server-simple.js`** - 主代理服务器（推荐使用，无外部依赖）
- **`server.js`** - 完整版代理服务器（需要 express 依赖）
- **`package.json`** - Node.js 项目配置
- **`启动代理服务器.bat`** - Windows 启动脚本
- **`test-proxy.js`** - 测试脚本

## 🚀 快速使用

### 方法1: 使用启动脚本（推荐）
```bash
双击运行: 启动代理服务器.bat
```

### 方法2: 手动启动
```bash
cd c:\Users\HP\Desktop\CIMC_xi\web\claude-relay-proxy
node server-simple.js
```

## ⚙️ VSCode 配置

已自动更新设置：
```json
{
  "relayMeter.apiUrl": "http://localhost:3001",
  "relayMeter.apiKey": "proxy-key"
}
```

## 🔄 使用步骤

1. **启动代理服务器**
   ```bash
   # 运行启动脚本或手动启动
   node server-simple.js
   ```

2. **重新加载 VSCode**
   - 按 `Ctrl + Shift + P`
   - 输入 `Developer: Reload Window`
   - 按回车执行

3. **检查插件状态**
   - 查看VSCode状态栏
   - 应该显示正常的API使用统计

## 🧪 测试功能

### 健康检查
```bash
curl http://localhost:3001/health
```

### API ID 测试
```bash
curl -X POST http://localhost:3001/apiStats/api/get-key-id \
     -H "Content-Type: application/json" \
     -d '{}'
```

### 使用量统计测试
```bash
curl -X POST http://localhost:3001/apiStats/api/get-usage-stats \
     -H "Content-Type: application/json" \
     -d '{}'
```

## 📊 功能特性

### ✅ 已实现
- [x] **API ID 获取**: 生成基于真实API Key的模拟ID
- [x] **使用量统计**: 获取真实的API使用量数据
- [x] **CORS支持**: 支持跨域请求
- [x] **错误处理**: 优雅的错误处理和日志记录
- [x] **健康检查**: 服务状态监控端点
- [x] **无依赖版本**: 使用Node.js内置模块，无需安装额外包

### 🔄 数据流程
```
VSCode插件 → 本地代理(:3001) → 真实API(code-next.akclau.de) → 返回数据
```

### 📈 数据来源
- **使用量数据**: 来自真实API (`/v1/dashboard/billing/usage`)
- **API ID**: 基于真实API Key生成的模拟ID
- **Token统计**: 基于使用量估算的近似值

## 🛠️ 技术实现

### 代理端点映射
| 插件请求 | 代理处理 | 真实API |
|---------|---------|---------|
| `POST /apiStats/api/get-key-id` | 生成模拟ID | 无 |
| `POST /apiStats/api/get-usage-stats` | 转换格式 | `GET /v1/dashboard/billing/usage` |
| `GET /health` | 服务状态 | 无 |

### 数据转换
```javascript
// 真实API响应
{ "total_usage": 80.35 }

// 转换为插件期望格式
{
  "success": true,
  "usage": { "total_usage": 80.35, "currency": "USD" },
  "tokens": { "input": 80350, "output": 40175 },
  "requests": 803,
  "cost": 80.35
}
```

## 🔧 故障排除

### 问题1: 端口被占用
```bash
# 查找占用端口的进程
netstat -ano | grep :3001

# 终止进程
taskkill /PID <进程ID> /F
```

### 问题2: 插件仍然显示错误
1. 确认代理服务器正在运行
2. 重新加载VSCode窗口
3. 检查代理服务器日志
4. 验证VSCode设置是否正确

### 问题3: 获取不到真实数据
- 检查网络连接
- 验证API Key是否有效
- 查看代理服务器错误日志

## 📝 开发说明

### 代理服务器特点
- **轻量级**: 使用Node.js内置模块
- **高性能**: 直接HTTP处理，无框架开销
- **兼容性**: 完全兼容Claude Relay Meter插件
- **可扩展**: 易于添加新的API端点

### 自定义配置
可以在 `server-simple.js` 中修改：
- 端口号：`const PORT = 3001;`
- API地址��`const API_BASE_URL = 'https://code-next.akclau.de';`
- API密钥：`const API_KEY = 'your-api-key';`

## 📞 支持

如果遇到问题：
1. 查看代理服务器控制台日志
2. 检查VSCode开发者工具的扩展日志
3. 使用测试脚本验证代理功能

---

**注意**: 代理服务器需要在后台持续运行以保持插件正常工作。建议创建开机自启动项或使用进程管理器。