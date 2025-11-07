/**
 * 智能终端管理系统 - 一键启动脚本
 * 同时启动WebSocket服务器和HTTP服务器
 */

const { spawn } = require('child_process');
const path = require('path');

// 颜色输出
const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    green: '\x1b[32m',
    blue: '\x1b[34m',
    yellow: '\x1b[33m',
    red: '\x1b[31m'
};

function log(color, prefix, message) {
    console.log(`${color}${prefix}${colors.reset} ${message}`);
}

// 存储子进程
const processes = [];

// 启动WebSocket服务器
function startWebSocketServer() {
    log(colors.blue, '[启动]', '正在启动 WebSocket 服务器...');

    const ws = spawn('node', ['server.js'], {
        cwd: __dirname,
        stdio: 'inherit'
    });

    ws.on('error', (err) => {
        log(colors.red, '[错误]', `WebSocket服务器启动失败: ${err.message}`);
    });

    ws.on('exit', (code) => {
        log(colors.yellow, '[退出]', `WebSocket服务器已退出 (代码: ${code})`);
    });

    processes.push(ws);
}

// 启动HTTP服务器
function startHTTPServer() {
    log(colors.blue, '[启动]', '正在启动 HTTP 服务器...');

    const http = spawn('node', ['http-server.js'], {
        cwd: __dirname,
        stdio: 'inherit'
    });

    http.on('error', (err) => {
        log(colors.red, '[错误]', `HTTP服务器启动失败: ${err.message}`);
    });

    http.on('exit', (code) => {
        log(colors.yellow, '[退出]', `HTTP服务器已退出 (代码: ${code})`);
    });

    processes.push(http);
}

// 优雅关闭所有进程
function shutdown() {
    log(colors.yellow, '[关闭]', '正在关闭所有服务器...');

    processes.forEach((proc) => {
        if (proc && !proc.killed) {
            proc.kill('SIGTERM');
        }
    });

    setTimeout(() => {
        log(colors.green, '[完成]', '所有服务器已关闭');
        process.exit(0);
    }, 1000);
}

// 监听退出信号
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);

// 主函数
function main() {
    console.log(colors.bright + '='.repeat(50) + colors.reset);
    console.log(colors.green + colors.bright + '  智能终端管理系统 - 服务启动器' + colors.reset);
    console.log(colors.bright + '='.repeat(50) + colors.reset);
    console.log();

    startWebSocketServer();
    setTimeout(() => {
        startHTTPServer();
    }, 1000);

    console.log();
    log(colors.green, '[提示]', '按 Ctrl+C 停止所有服务器');
    console.log();
}

// 执行
main();
