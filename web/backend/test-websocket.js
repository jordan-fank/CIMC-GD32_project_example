/**
 * WebSocket连接测试脚本
 */

const WebSocket = require('ws');

console.log('正��测试WebSocket连接到 localhost:8080...');

const ws = new WebSocket('ws://localhost:8080');

ws.on('open', () => {
    console.log('✅ WebSocket连接成功！');

    // 发送测试消息
    ws.send(JSON.stringify({
        type: 'test',
        message: '这是一个测试消息',
        timestamp: new Date().toISOString()
    }));

    // 3秒后关闭连接
    setTimeout(() => {
        ws.close();
    }, 3000);
});

ws.on('message', (data) => {
    console.log('✅ 收到消息:', data.toString());
});

ws.on('close', () => {
    console.log('✅ WebSocket连接已关闭');
    process.exit(0);
});

ws.on('error', (error) => {
    console.error('❌ WebSocket连接失败:', error.message);
    process.exit(1);
});

// 10秒超时
setTimeout(() => {
    console.error('❌ 连接超时');
    process.exit(1);
}, 10000);