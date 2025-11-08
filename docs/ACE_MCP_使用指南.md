# ACE MCP 代码库索引系统使用指南

## 🚀 功能概述

ACE MCP是一个强大的代码库索引和语义搜索工具，现已集成到你的Claude Code环境中。它能够：

- **自动增量索引**：实���跟踪项目文件变化
- **语义搜索**：根据自然语言查询搜索相关代码
- **智能编码支持**：支持UTF-8、GBK、GB2312等多种编码
- **Web管理界面**：提供可视化的管理界面

## 📁 配置文件位置

- **MCP配置**：`C:\Users\HP\.config\claude-code\mcp.json`
- **ACE配置**：`C:\Users\HP\.acemcp\settings.toml`

## 🔧 配置说明

### 1. API配置 (需要配置)

编辑 `C:\Users\HP\.acemcp\settings.toml`：

```toml
# API配置 - 请替换为你的实际API信息
BASE_URL = "https://api.openai.com/v1"  # 或其他兼容的API端点
TOKEN = "your-api-token-here"           # 你的API令牌
```

### 2. 索引优化配置

```toml
# 批处理设置 - 已针对GD32项目优化
BATCH_SIZE = 15                    # 每批处理的文件数量
MAX_LINES_PER_BLOB = 1000         # 大文件分割行数
```

## 🎯 支持的文件类型

已针对你的GD32嵌入式项目优化，支持：

- **C/C++嵌入式文件**：`.c`, `.h`, `.cpp`, `.hpp`, `.s`, `.S`, `.asm`
- **构建配置**：`Makefile`, `.cmake`, `.mk`, `.lds`, `.ld`
- **Web调试界面**：`.js`, `.html`, `.css`, `.json`
- **文档和配置**：`.md`, `.toml`, `.yaml`, `.txt`

## 🚫 自动排除的文件/目录

- 构建输出：`build/`, `Debug/`, `Release/`, `*.o`, `*.elf`
- 编译器文件：`.vscode/`, `.idea/`, `.mxproject`
- 缓存目录：`__pycache__/`, `CMakeFiles/`
- 版本控制：`.git/`, `.svn/`

## 📖 使用方法

### 1. 通过Claude Code使用

配置完成后，可以直接在Claude Code中使用语义搜索功能：

```
请搜索项目中与LED控制相关的代码
查找WebSocket通信的实现
搜索所有HAL相关的初始化代码
```

### 2. Web管理界面 (可选)

启动Web管理界面查看索引状态：

```bash
"C:/Users/HP/.local/bin/acemcp.exe" --web-port 8080
```

然后访问 http://localhost:8080

## ⚙️ 首次使用设置

1. **配置API令牌**：
   编辑 `C:\Users\HP\.acemcp\settings.toml`
   将 `TOKEN = "your-api-token-here"` 替换为你的实际API令牌

2. **重启Claude Code**：
   配置完成后重启Claude Code以加载新的MCP服务器

3. **开始使用**：
   直接向Claude提问关于代码的问题，系统会自动进行语义搜索

## 🔍 搜索示例

- "搜索所有与4G通信相关的代码文件"
- "查找LED控制相关的函数和实现"
- "搜索WebSocket协议处理的具体实现"
- "查找GD32芯片的HAL配置代码"

## 📝 注意事项

1. **API配置**：首次使用前必须配置有效的API令牌
2. **网络连接**：确保能访问配置的API端点
3. **项目路径**：在项目根目录使用Claude Code以获得最佳效果
4. **增量更新**：系统会自动检测文件变化并增量更新索引

## 🛠️ 故障排除

### 如果搜索无结果：
1. 检查API令牌是否正确配置
2. 确认网络连接正常
3. 查看acemcp日志输出

### 如果配置错误：
1. 检查 `mcp.json` 中的路径是否正确
2. 验证acemcp.exe文件是否存在
3. 确认配置文件格式有效

---

🎉 **配置完成！** 你的GD32项目现在具备了强大的代码语义搜索能力。