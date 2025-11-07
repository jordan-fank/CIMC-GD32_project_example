/**
 * 智能终端管理系统 - WebSocket服务器（恢复原版）
 * 功能：接收客户端数据后转发给所有已连接的客户端
 */

const WebSocket = require('ws');

// 配置参数
const WS_PORT = 8080;  // WebSocket服务端口（恢复原有配置）
const HEARTBEAT_INTERVAL = 30000;  // 心跳检测间隔（30秒）

// 存储所有已连接的客户端
const clients = new Set();

// 创建WebSocket服务器
const wss = new WebSocket.Server({
    port: WS_PORT,
    path: '/'
});

console.log('==========================================');
console.log('  智能终端管理系统 - WebSocket服务器');
console.log('==========================================');
console.log(`WebSocket服务启动成功，监听端口: ${WS_PORT}`);
console.log('==========================================');

/**
 * 处理客户端连接
 */
wss.on('connection', (ws, req) => {
    console.log(`[${new Date().toISOString()}] 新客户端连接: ${req.socket.remoteAddress}`);

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
            console.log(`[${new Date().toISOString()}] 收到消息，转发给 ${clients.size - 1} 个客户端`);

            // 转发给所有其他客户端
            broadcastToAll(data, ws);

        } catch (error) {
            console.error('消息处理错误:', error);
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
        console.log(`[${new Date().toISOString()}] 客户端断开连接: ${code} - ${reason}`);
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
        console.error('WebSocket错误:', error);
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
 * 向��有客户端广播消息
 * @param {string} data - 要广播的消息
 * @param {WebSocket} exclude - 要排除的客户端（发送者）
 */
function broadcastToAll(data, exclude = null) {
    const message = data.toString();

    clients.forEach((client) => {
        // 排除发送者，发送给其他所有客户端
        if (client !== exclude && client.readyState === WebSocket.OPEN) {
            try {
                client.send(message);
            } catch (error) {
                console.error('发送消息失败:', error);
                clients.delete(client);
            }
        }
    });
}

/**
 * 获取服务器状态
 */
wss.on('listening', () => {
    console.log(`服务器监听端口: ${WS_PORT}`);
    console.log(`支持的心跳间隔: ${HEARTBEAT_INTERVAL}ms`);
});

/**
 * 处理服务器错误
 */
wss.on('error', (error) => {
    console.error('服务器错误:', error);
});

// 优雅关闭
process.on('SIGINT', () => {
    console.log('\n正在关闭WebSocket服务器...');

    // 通知所有客户端服务器即将关闭
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
    wss.close(() => {
        console.log('WebSocket服务器已关闭');
        process.exit(0);
    });
});

process.on('SIGTERM', () => {
    console.log('\n收到终止信号，正在关闭服务器...');
    wss.close(() => {
        console.log('WebSocket服务器已关闭');
        process.exit(0);
    });
});

// 导出广播函数供其他模块使用
module.exports = {
    broadcastToAll,
    getClientCount: () => clients.size
};