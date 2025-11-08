/**
 * 智能终端管理系统 - 统一服务器
 * 同时提供HTTP静态文件服务和WebSocket服务
 * 解决移动设备WebSocket连接问题
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const WebSocket = require('ws');

// 配置参数
const HTTP_PORT = 8080;  // HTTP服务端口（匹配FRP配置）
const FRONTEND_DIR = path.join(__dirname, '../frontend');  // 前端文件目录
const HEARTBEAT_INTERVAL = 30000;  // 心跳检测间隔（30秒）

// 存储所有已连接的WebSocket客户端
const clients = new Set();

// MIME类型映射
const MIME_TYPES = {
    '.html': 'text/html; charset=utf-8',
    '.css': 'text/css; charset=utf-8',
    '.js': 'application/javascript; charset=utf-8',
    '.json': 'application/json; charset=utf-8',
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.gif': 'image/gif',
    '.svg': 'image/svg+xml',
    '.ico': 'image/x-icon',
    '.txt': 'text/plain; charset=utf-8'
};

/**
 * 获取文件的MIME类型
 */
function getMimeType(filePath) {
    const ext = path.extname(filePath).toLowerCase();
    return MIME_TYPES[ext] || 'application/octet-stream';
}

/**
 * 创建HTTP服务器
 */
const server = http.createServer((req, res) => {
    console.log(`[HTTP] ${req.method} ${req.url}`);

    // 处理WebSocket升级请求
    if (req.url === '/ws' && req.headers.upgrade === 'websocket') {
        console.log(`[WebSocket] 升级请求: ${req.url}`);
        wss.handleUpgrade(req, req.socket, Buffer.alloc(0), (ws) => {
            wss.emit('connection', ws, req);
        });
        return;
    }

    // 解析请求路径
    let requestPath = req.url === '/' ? '/index.html' : req.url;

    // 移除查询参数
    const queryIndex = requestPath.indexOf('?');
    if (queryIndex !== -1) {
        requestPath = requestPath.substring(0, queryIndex);
    }

    // 构建完整文件路径
    const filePath = path.join(FRONTEND_DIR, requestPath);

    // 安全检查：防止路径遍历攻击
    const normalizedPath = path.normalize(filePath);
    if (!normalizedPath.startsWith(FRONTEND_DIR)) {
        res.writeHead(403, { 'Content-Type': 'text/plain; charset=utf-8' });
        res.end('禁止访问');
        console.log(`[拒绝] 非法路径访问尝试: ${req.url}`);
        return;
    }

    // 读取并返回文件
    fs.readFile(filePath, (err, data) => {
        if (err) {
            if (err.code === 'ENOENT') {
                // 文件不存在 - 返回404页面
                res.writeHead(404, { 'Content-Type': 'text/html; charset=utf-8' });
                res.end(`
                    <!DOCTYPE html>
                    <html lang="zh-CN">
                    <head>
                        <meta charset="UTF-8">
                        <meta name="viewport" content="width=device-width, initial-scale=1.0">
                        <title>404 - 页面不存在</title>
                        <style>
                            body {
                                font-family: 'Microsoft YaHei', Arial, sans-serif;
                                background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
                                display: flex;
                                justify-content: center;
                                align-items: center;
                                height: 100vh;
                                margin: 0;
                                color: white;
                            }
                            .container {
                                text-align: center;
                            }
                            h1 {
                                font-size: 72px;
                                margin: 0;
                            }
                            p {
                                font-size: 24px;
                            }
                            a {
                                color: white;
                                text-decoration: underline;
                            }
                        </style>
                    </head>
                    <body>
                        <div class="container">
                            <h1>404</h1>
                            <p>页面不存在</p>
                            <a href="/">返回首页</a>
                        </div>
                    </body>
                    </html>
                `);
                console.log(`[404] 文件不存在: ${requestPath}`);
            } else {
                // 其他错误
                res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8' });
                res.end('服务器内部错误');
                console.error(`[错误] 读取文件失败 (${requestPath}):`, err.message);
            }
            return;
        }

        // 成功返回文件
        const mimeType = getMimeType(filePath);
        res.writeHead(200, { 'Content-Type': mimeType });
        res.end(data);
        console.log(`[200] ${req.method} ${requestPath} (${mimeType})`);
    });
});

/**
 * 创建WebSocket服务器
 */
const wss = new WebSocket.Server({
    noServer: true,
    path: '/ws'
});

/**
 * 处理WebSocket连接
 */
wss.on('connection', (ws, req) => {
    console.log(`[WebSocket] 新客户端连接: ${req.socket.remoteAddress}`);

    // 添加到客户端列表
    clients.add(ws);

    // 发送连接成功消息
    ws.send(JSON.stringify({
        type: 'system',
        message: '连接成功'
    }));

    /**
     * 处理客户端消息
     */
    ws.on('message', (data) => {
        try {
            console.log(`[WebSocket] 收到消息，转发给所有 ${clients.size} 个客户端`);

            // 转发给所有客户端（包括发送者）
            broadcastToAll(data, null);

        } catch (error) {
            console.error('[WebSocket] 消息处理错误:', error);
            ws.send(JSON.stringify({
                type: 'error',
                message: '消息处理失败'
            }));
        }
    });

    /**
     * 处理连接关闭
     */
    ws.on('close', (code, reason) => {
        console.log(`[WebSocket] 客户端断开连接: ${code} - ${reason}`);
        clients.delete(ws);

        // 通知其他客户端
        broadcastToAll(JSON.stringify({
            type: 'system',
            message: '有客户端断开连接'
        }));
    });

    /**
     * 处理连接错误
     */
    ws.on('error', (error) => {
        console.error('[WebSocket] 错误:', error);
        clients.delete(ws);
    });

    // 发送心跳检测
    const heartbeat = setInterval(() => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.ping();
        } else {
            clearInterval(heartbeat);
            clients.delete(ws);
        }
    }, HEARTBEAT_INTERVAL);
});

/**
 * 向所有客户端广播消息
 * @param {string} data - 要广播的消息
 * @param {WebSocket} exclude - 要排除的客户端（发送者），设为null则发送给所有人
 */
function broadcastToAll(data, exclude = null) {
    const message = data.toString();

    clients.forEach((client) => {
        // 如果设置了排除参数，则排除指定客户端；否则发送给所有人
        if (exclude === null || client !== exclude) {
            if (client.readyState === WebSocket.OPEN) {
                try {
                    client.send(message);
                } catch (error) {
                    console.error('[WebSocket] 发送消息失败:', error);
                    clients.delete(client);
                }
            }
        }
    });
}

// 启动服务器
server.listen(HTTP_PORT, () => {
    console.log('==========================================');
    console.log('  智能终端管理系统 - 统一服务器');
    console.log('==========================================');
    console.log(`服务器启动成功！`);
    console.log(`HTTP访问地址: http://localhost:${HTTP_PORT}`);
    console.log(`WebSocket地址: ws://localhost:${HTTP_PORT}/ws`);
    console.log(`前端目录: ${FRONTEND_DIR}`);
    console.log('==========================================');
    console.log('\n提示: 现在可以通过同一个端口访问HTTP和WebSocket服务');
    console.log('移动设备应该能够正常连接WebSocket服务\n');
});

// 优雅关闭处理
process.on('SIGINT', () => {
    console.log('\n[服务器] 正在关闭统一服务器...');

    // 通知所有WebSocket客户端服务器即将关闭
    clients.forEach((client) => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify({
                type: 'system',
                message: '服务器即将关闭'
            }));
            client.close();
        }
    });

    // 关闭服务器
    server.close(() => {
        console.log('[服务器] 统一服务器已安全关闭');
        process.exit(0);
    });
});

process.on('SIGTERM', () => {
    console.log('\n[服务器] 收到终止信号，关闭中...');
    server.close(() => {
        process.exit(0);
    });
});

// 导出函数供其他模块使用
module.exports = {
    broadcastToAll,
    getClientCount: () => clients.size
};