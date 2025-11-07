/**
 * 智能终端管理系统 - 统一启动器
 * 启动集成HTTP+WebSocket的统一服务器
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
    red: '\x1b[31m',
    cyan: '\x1b[36m'
};

function log(color, prefix, message) {
    console.log(`${color}${prefix}${colors.reset} ${message}`);
}

// 存储子进程
const processes = [];

// 启动统一服务器
function startUnifiedServer() {
    log(colors.blue, '[启动]', '正在启动统一服务器 (HTTP + WebSocket)...');

    const server = spawn('node', ['server-unified.js'], {
        cwd: __dirname,
        stdio: 'inherit'
    });

    server.on('error', (err) => {
        log(colors.red, '[错误]', `服务器启动失败: ${err.message}`);
        process.exit(1);
    });

    server.on('exit', (code) => {
        log(colors.yellow, '[退出]', `服务器已退出 (代码: ${code})`);
    });

    processes.push(server);
    return server;
}

// 优雅关闭所有进程
function shutdown() {
    log(colors.yellow, '[���闭]', '正在关闭服务器...');

    processes.forEach((proc) => {
        if (proc && !proc.killed) {
            proc.kill('SIGTERM');
        }
    });

    setTimeout(() => {
        log(colors.green, '[完成]', '服务器已关闭');
        process.exit(0);
    }, 1000);
}

// 监听退出信号
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);

// 主函数
function main() {
    console.log(colors.bright + '='.repeat(60) + colors.reset);
    console.log(colors.green + colors.bright + '  智能终端管理系统 - 统一服务器' + colors.reset);
    console.log(colors.bright + '='.repeat(60) + colors.reset);
    console.log();

    log(colors.cyan, '[信息]', '集成的HTTP + WebSocket服务器');
    log(colors.cyan, '[信息]', '支持网页访问和实时通信');
    log(colors.cyan, '[信息]', '自动检测域名并配置连接参数');
    console.log();

    const server = startUnifiedServer();

    // 等待服务器启动
    setTimeout(() => {
        console.log();
        log(colors.green, '[✓ 成功]', '服务器启动完成！');
        console.log();
        log(colors.blue, '[访问]', '本地地址: http://localhost:8080');
        log(colors.blue, '[访问]', '公网地址: http://www.yanjin.xyz (配置FRP后)');
        console.log();
        log(colors.yellow, '[提示]', '按 Ctrl+C 停止服务器');
        console.log();
    }, 2000);
}

// 执行
main();