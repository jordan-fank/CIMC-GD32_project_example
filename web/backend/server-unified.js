/**
 * 智能终端管理系统 - 统一服务器
 * 集成HTTP静态文件服务和WebSocket通信
 */

const WebSocket = require('ws');
const http = require('http');
const fs = require('fs');
const path = require('path');

// 配置参数
const PORT = 8080;
const HEARTBEAT_INTERVAL = 30000;
const FRONTEND_DIR = path.join(__dirname, '../frontend');

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

// 存储所有已连接的WebSocket客户端
const clients = new Map();

// 获���文件的MIME类型
function getMimeType(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  return MIME_TYPES[ext] || 'application/octet-stream';
}

// 创建HTTP服务器
const server = http.createServer((req, res) => {
  let requestPath = req.url === '/' ? '/index.html' : req.url;

  // 移除查询参数
  const queryIndex = requestPath.indexOf('?');
  if (queryIndex !== -1) {
    requestPath = requestPath.substring(0, queryIndex);
  }

  // 安全检查：防止目录遍历攻击
  const safePath = path.normalize(requestPath).replace(/^(\.\.[\/\\])+/, '');
  const filePath = path.join(FRONTEND_DIR, safePath);

  // 检查文件是否存在且在允许的目录内
  if (!filePath.startsWith(FRONTEND_DIR)) {
    res.writeHead(403, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end('403 Forbidden');
    return;
  }

  // 读取文件
  fs.readFile(filePath, (err, data) => {
    if (err) {
      if (err.code === 'ENOENT') {
        // 文件不存在，返回404
        res.writeHead(404, { 'Content-Type': 'text/html; charset=utf-8' });
        res.end(`
          <!DOCTYPE html>
          <html>
          <head>
            <meta charset="UTF-8">
            <title>404 - 页面未找到</title>
            <style>
              body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }
              h1 { color: #e74c3c; }
            </style>
          </head>
          <body>
            <h1>404 - 页面未找到</h1>
            <p>请求的页面不存在</p>
            <a href="/">返回首页</a>
          </body>
          </html>
        `);
      } else {
        // 其他错误
        console.error('文件读取错误:', err);
        res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8' });
        res.end('500 Internal Server Error');
      }
      return;
    }

    // 设置响应头
    const mimeType = getMimeType(filePath);
    res.writeHead(200, {
      'Content-Type': mimeType,
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Methods': 'GET, POST, PUT, DELETE, OPTIONS',
      'Access-Control-Allow-Headers': 'Origin, X-Requested-With, Content-Type, Accept, Authorization'
    });

    res.end(data);
  });
});

// 创建WebSocket服务器（升级HTTP连接）
const wss = new WebSocket.Server({
  server
});

// WebSocket连接处理
wss.on('connection', (ws, req) => {
  const clientId = generateClientId();
  const clientInfo = {
    id: clientId,
    ws: ws,
    connectedAt: Date.now(),
    ip: req.socket.remoteAddress,
    userAgent: req.headers['user-agent']
  };

  clients.set(clientId, clientInfo);

  console.log(`[${new Date().toISOString()}] 新客户端连接: ${clientId} (${clientInfo.ip})`);

  // 发送连接成功消息
  sendToClient(ws, {
    type: 'system',
    message: '连接成功',
    clientId: clientId,
    timestamp: Date.now(),
    clientCount: clients.size
  });

  // 通知其他客户端有新连接
  broadcastToAll({
    type: 'system',
    message: `新客户端加入 (${clientId})`,
    timestamp: Date.now(),
    clientCount: clients.size
  }, clientId);

  // 处理客户端消息
  ws.on('message', (data) => {
    try {
      console.log(`[${new Date().toISOString()}] 收到消息: ${clientId}`);

      // 解析消息
      let message;
      try {
        message = JSON.parse(data.toString());
      } catch (e) {
        message = {
          type: 'text',
          content: data.toString(),
          timestamp: Date.now(),
          clientId: clientId
        };
      }

      // 添加客户端ID和时间戳
      message.clientId = clientId;
      message.timestamp = Date.now();

      // 广播给所有其他客户端
      broadcastToAll(message, clientId);

    } catch (error) {
      console.error('消息处理错误:', error);
      sendToClient(ws, {
        type: 'error',
        message: '消息处理失败',
        timestamp: Date.now()
      });
    }
  });

  // 处理连接关闭
  ws.on('close', (code, reason) => {
    clients.delete(clientId);
    console.log(`[${new Date().toISOString()}] 客户端断开: ${clientId} (${code} - ${reason})`);

    // 通知其他客户端
    broadcastToAll({
      type: 'system',
      message: `客户端离开 (${clientId})`,
      timestamp: Date.now(),
      clientCount: clients.size
    });
  });

  // 处理连接错误
  ws.on('error', (error) => {
    console.error('WebSocket错误:', error);
    clients.delete(clientId);
  });

  // 心跳检测
  const heartbeat = setInterval(() => {
    if (ws.readyState === WebSocket.OPEN) {
      ws.ping();
    } else {
      clearInterval(heartbeat);
      clients.delete(clientId);
    }
  }, HEARTBEAT_INTERVAL);

  // 清理心跳
  ws.on('close', () => {
    clearInterval(heartbeat);
  });
});

// 生成客户端ID
function generateClientId() {
  return 'client_' + Math.random().toString(36).substr(2, 9);
}

// 发送消息给指定客户端
function sendToClient(ws, data) {
  if (ws.readyState === WebSocket.OPEN) {
    try {
      ws.send(JSON.stringify(data));
    } catch (error) {
      console.error('发送消息失败:', error);
    }
  }
}

// 向所有客户端广播消息
function broadcastToAll(data, excludeClientId = null) {
  const message = JSON.stringify(data);

  clients.forEach((clientInfo, clientId) => {
    // 排除发送者
    if (clientId !== excludeClientId && clientInfo.ws.readyState === WebSocket.OPEN) {
      try {
        clientInfo.ws.send(message);
      } catch (error) {
        console.error('广播消息失败:', error);
        clients.delete(clientId);
      }
    }
  });
}

// 导出函数供外部使用
module.exports = {
  broadcastToAll,
  getClientCount: () => clients.size,
  getClients: () => Array.from(clients.entries()).map(([id, info]) => ({
    id,
    ip: info.ip,
    connectedAt: info.connectedAt,
    userAgent: info.userAgent
  }))
};

// 服务器启动日志
server.listen(PORT, () => {
  console.log('==========================================');
  console.log('  智能终端管理系统 - 统一服务器');
  console.log('==========================================');
  console.log(`✓ HTTP服务: http://localhost:${PORT}`);
  console.log(`✓ WebSocket服务: ws://localhost:${PORT}`);
  console.log(`✓ 前端目录: ${FRONTEND_DIR}`);
  console.log(`✓ 心跳间隔: ${HEARTBEAT_INTERVAL}ms`);
  console.log('==========================================');
  console.log();
  console.log('🌐 公网访问配置:');
  console.log('   1. 配置FRP HTTP映射到端口' + PORT);
  console.log('   2. 域名指向此服务器');
  console.log('   3. 访问: http://www.yanjin.xyz');
  console.log('==========================================');
});

// 优雅关闭
process.on('SIGINT', () => {
  console.log('\n正在关闭统一服务器...');

  // 通知所有客户端服务器即将关闭
  broadcastToAll({
    type: 'system',
    message: '服务器即将关闭',
    timestamp: Date.now()
  });

  // 关闭所有WebSocket连接
  clients.forEach((clientInfo) => {
    if (clientInfo.ws.readyState === WebSocket.OPEN) {
      clientInfo.ws.close();
    }
  });

  // 关闭HTTP服务器
  server.close(() => {
    console.log('统一服务器已关闭');
    process.exit(0);
  });
});

process.on('SIGTERM', () => {
  console.log('\n收到终止信号，正在关闭服务器...');
  server.close(() => {
    console.log('统一服务器已关闭');
    process.exit(0);
  });
});