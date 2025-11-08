/**
 * 智能终端管理系统 - 统一服务器启动脚本
 * 启动集成HTTP和WebSocket服务的统一服务器
 */

const { spawn } = require('child_process');

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

// 主函数
function main() {
    console.log(colors.bright + '='.repeat(50) + colors.reset);
    console.log(colors.green + colors.bright + '  智能终端管理系统 - 统一服务器' + colors.reset);
    console.log(colors.bright + '='.repeat(50) + colors.reset);
    console.log();

    log(colors.blue, '[启动]', '正在启动统一服务器（HTTP + WebSocket）...');

    const unifiedServer = spawn('node', ['unified-server.js'], {
        cwd: __dirname,
        stdio: 'inherit'
    });

    unifiedServer.on('error', (err) => {
        log(colors.red, '[错误]', `统一服务器启动失败: ${err.message}`);
    });

    unifiedServer.on('exit', (code) => {
        log(colors.yellow, '[退出]', `统一服务器已退出 (代码: ${code})`);
    });

    // 监听退出信号
    process.on('SIGINT', () => {
        log(colors.yellow, '[关闭]', '正在关闭统一服务器...');
        if (unifiedServer && !unifiedServer.killed) {
            unifiedServer.kill('SIGTERM');
        }
        setTimeout(() => {
            log(colors.green, '[完成]', '统一服务器已关闭');
            process.exit(0);
        }, 1000);
    });

    process.on('SIGTERM', () => {
        log(colors.yellow, '[关闭]', '收到终止信号，正在关闭...');
        if (unifiedServer && !unifiedServer.killed) {
            unifiedServer.kill('SIGTERM');
        }
        setTimeout(() => {
            process.exit(0);
        }, 1000);
    });

    console.log();
    log(colors.green, '[提示]', '按 Ctrl+C 停止服务器');
    log(colors.green, '[说明]', 'HTTP和WebSocket现在运行在同一端口8080');
    log(colors.green, '[说明]', '移动设备应该能够正常连接WebSocket服务');
    console.log();
}

// 执行
main();