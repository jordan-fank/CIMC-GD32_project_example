/**
 * Claude Relay Meter 代理服务器
 * 将插件的API请求转换为OpenAI兼容格式
 */

const express = require('express');
const http = require('http');
const app = express();
const port = 3001; // 本地代理端口

// 中间件
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// CORS支持
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
    res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept, Authorization');
    if (req.method === 'OPTIONS') {
        res.sendStatus(200);
    } else {
        next();
    }
});

// API配置
const API_BASE_URL = 'https://code-next.akclau.de';
const API_KEY = 'sk-emvvPrWnQq3FKLTfLNqi6yF8kHzxqK9FGogwx1rkhlGKFChg';

// 代理函数
async function proxyRequest(endpoint, method = 'GET', data = null) {
    const url = `${API_BASE_URL}${endpoint}`;
    const options = {
        method: method,
        headers: {
            'Authorization': `Bearer ${API_KEY}`,
            'Content-Type': 'application/json',
            'User-Agent': 'Claude-Relay-Meter-Proxy/1.0'
        }
    };

    if (data && method !== 'GET') {
        options.body = JSON.stringify(data);
    }

    try {
        const response = await fetch(url, options);
        const responseData = await response.json();

        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${responseData.error?.message || 'Unknown error'}`);
        }

        return responseData;
    } catch (error) {
        console.error(`代理请求失败: ${url}`, error.message);
        throw error;
    }
}

// 插件兼容的API端点

/**
 * 获取API ID - 返回模拟的API ID
 * 插件期望的格式: {"apiId": "some-id"}
 */
app.post('/apiStats/api/get-key-id', async (req, res) => {
    try {
        console.log('[代理] 收到获取API ID请求');

        // 生成一个基于API Key的模拟ID
        const apiId = `user_${API_KEY.substring(0, 8)}_${Date.now()}`;

        const response = {
            success: true,
            apiId: apiId,
            message: "API ID获取成功（代理模式）"
        };

        console.log('[代理] 返回API ID:', apiId);
        res.json(response);

    } catch (error) {
        console.error('[代理] 获取API ID失败:', error.message);
        res.status(500).json({
            success: false,
            error: error.message
        });
    }
});

/**
 * 获取使用量统计
 * 插件期望的格式: 包含usage、tokens、cost等字段
 */
app.post('/apiStats/api/get-usage-stats', async (req, res) => {
    try {
        console.log('[代理] 收到获取使用量统计请求');

        // 从OpenAI兼容端点获取真实使用量
        const usageData = await proxyRequest('/v1/dashboard/billing/usage');

        // 转换为插件期望的格式
        const stats = {
            success: true,
            usage: {
                total_usage: usageData.total_usage || 0,
                currency: 'USD',
                period: {
                    start: new Date(Date.now() - 30 * 24 * 60 * 60 * 1000).toISOString(),
                    end: new Date().toISOString()
                }
            },
            tokens: {
                input: Math.floor((usageData.total_usage || 0) * 1000), // 估算
                output: Math.floor((usageData.total_usage || 0) * 500)  // 估算
            },
            requests: Math.floor((usageData.total_usage || 0) * 10), // 估算请求数
            cost: usageData.total_usage || 0,
            lastUpdated: new Date().toISOString()
        };

        console.log('[代理] 返回使用量统计:', JSON.stringify(stats, null, 2));
        res.json(stats);

    } catch (error) {
        console.error('[代理] 获取使用量统计失败:', error.message);

        // 返回默认数据
        const defaultStats = {
            success: true,
            usage: {
                total_usage: 0,
                currency: 'USD',
                period: {
                    start: new Date().toISOString(),
                    end: new Date().toISOString()
                }
            },
            tokens: { input: 0, output: 0 },
            requests: 0,
            cost: 0,
            lastUpdated: new Date().toISOString(),
            note: "无法获取实时数据，显示默认值"
        };

        res.json(defaultStats);
    }
});

/**
 * 健康检查端点
 */
app.get('/health', (req, res) => {
    res.json({
        status: 'ok',
        service: 'Claude Relay Meter 代理',
        version: '1.0.0',
        timestamp: new Date().toISOString()
    });
});

// 错误处理
app.use((err, req, res, next) => {
    console.error('[代理] 服务器错误:', err);
    res.status(500).json({
        success: false,
        error: '服务器内部错误'
    });
});

// 404处理
app.use((req, res) => {
    console.log('[代理] 未知端点:', req.method, req.path);
    res.status(404).json({
        success: false,
        error: `端点不存在: ${req.method} ${req.path}`
    });
});

// 启动服务器
const server = http.createServer(app);
server.listen(port, 'localhost', () => {
    console.log('========================================');
    console.log('  Claude Relay Meter 代理服务器');
    console.log('========================================');
    console.log(`✓ 代理地址: http://localhost:${port}`);
    console.log(`✓ 目标API: ${API_BASE_URL}`);
    console.log(`✓ 健康检查: http://localhost:${port}/health`);
    console.log('========================================');
    console.log('');
    console.log('请更新VSCode设置:');
    console.log(`"relayMeter.apiUrl": "http://localhost:${port}"`);
    console.log('"relayMeter.apiKey": "any-key-or-leave-empty"');
    console.log('========================================');
});

// 优雅关闭
process.on('SIGINT', () => {
    console.log('\n[代理] 正在关闭服务器...');
    server.close(() => {
        console.log('[代理] 服务器已关闭');
        process.exit(0);
    });
});

process.on('SIGTERM', () => {
    console.log('\n[代理] 收到终止信号...');
    server.close(() => {
        console.log('[代理] 服务器已关闭');
        process.exit(0);
    });
});