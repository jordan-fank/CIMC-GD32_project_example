/**
 * 智能终端管理系统 - HTTP静态文件服务器
 * 用于部署前端页面
 */

const http = require('http');
const fs = require('fs');
const path = require('path');

// 配置参数
const HTTP_PORT = 3000;  // HTTP服务端口
const FRONTEND_DIR = path.join(__dirname, '../frontend');  // 前端文件目录

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
 * @param {string} filePath - 文件路径
 * @returns {string} MIME类型
 */
function getMimeType(filePath) {
    const ext = path.extname(filePath).toLowerCase();
    return MIME_TYPES[ext] || 'application/octet-stream';
}

/**
 * 创建HTTP服务器
 */
const server = http.createServer((req, res) => {
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
                // 文件不存在
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

// 启动服务器
server.listen(HTTP_PORT, () => {
    console.log('===========================================');
    console.log('  智能终端管理系统 - HTTP服务器');
    console.log('===========================================');
    console.log(`  访问地址: http://localhost:${HTTP_PORT}`);
    console.log(`  前端目录: ${FRONTEND_DIR}`);
    console.log('===========================================');
    console.log('\n提示: 请确保WebSocket服务器也在运行中\n');
});

// 优雅关闭处理
process.on('SIGINT', () => {
    console.log('\n[服务器] 正在关闭HTTP服务器...');
    server.close(() => {
        console.log('[服务器] HTTP服务器已安全关闭');
        process.exit(0);
    });
});

process.on('SIGTERM', () => {
    console.log('\n[服务器] 收到终止信号，关闭中...');
    server.close(() => {
        process.exit(0);
    });
});
