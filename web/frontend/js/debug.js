/**
 * 智能终端调试系统 - JavaScript控制脚本
 * 功能：LED控制、WebSocket通信、界面管理
 */

class LEDDebugSystem {
    constructor() {
        this.ws = null;
        this.isConnected = false;
        this.ledStates = [0, 0, 0, 0, 0, 0]; // 6个LED的状态，0=关闭，1=开启
        this.reconnectInterval = null;
        this.pendingCommands = []; // 待发送的命令队列
        this.lastCommandTime = {}; // 防抖：记录每个LED的最后命令时间
        this.COMMAND_DEBOUNCE_TIME = 500; // 500ms防抖时间

        this.init();
    }

    /**
     * 初始化系统
     */
    init() {
        this.initWebSocket();
        this.updateAllLEDIndicators();
        this.updateConnectionStatus(false);
    }

    /**
     * 初始化WebSocket连接
     */
    initWebSocket() {
        // 自动检测服务器地址
        const host = window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1'
            ? 'localhost'
            : 'hk-4.lcf.im';
        const port = window.location.hostname === 'localhost' ? 47315 : 47315;

        const wsUrl = `ws://${host}:${port}`;

        try {
            console.log(`[调试系统] 正在连接到WebSocket服务器: ${wsUrl}`);
            this.ws = new WebSocket(wsUrl);

            // 连接成功事件
            this.ws.onopen = () => {
                this.isConnected = true;
                this.updateConnectionStatus(true);
                console.log('[调试系统] WebSocket连接成功');

                // 停止重连
                this.stopReconnect();

                // 发送待发送队列中的命令
                this.sendPendingCommands();
            };

            // 接收消息事件
            this.ws.onmessage = (event) => {
                this.handleMessage(event.data);
            };

            // 连接关闭事件
            this.ws.onclose = () => {
                this.isConnected = false;
                this.updateConnectionStatus(false);
                console.log('[调试系统] WebSocket连接已断开');

                // 启动自动重连
                this.startReconnect();
            };

            // 连接错误事件
            this.ws.onerror = (error) => {
                console.error('[调试系统] WebSocket连接错误:', error);
                this.isConnected = false;
                this.updateConnectionStatus(false);
            };

        } catch (error) {
            console.error('[调试系统] 创建WebSocket连接失败:', error);
            this.isConnected = false;
            this.updateConnectionStatus(false);
        }
    }

    /**
     * 控制LED
     * @param {number} ledNum - LED编号 (1-6)
     * @param {number} state - 状态 (0=关闭, 1=开启)
     */
    controlLED(ledNum, state) {
        if (ledNum < 1 || ledNum > 6) {
            console.error('[调试系统] LED编号错误:', ledNum);
            return;
        }

        if (state !== 0 && state !== 1) {
            console.error('[调试系统] LED状态错误:', state);
            return;
        }

        // 防抖检查：如果相同LED的相同状态在短时间内重复发送，则忽略
        const commandKey = `LED${ledNum}_${state}`;
        const now = Date.now();
        if (this.lastCommandTime[commandKey] &&
            (now - this.lastCommandTime[commandKey]) < this.COMMAND_DEBOUNCE_TIME) {
            console.log(`[调试系统] 防抖忽略重复命令: ${commandKey}`);
            return;
        }

        // 记录命令时间
        this.lastCommandTime[commandKey] = now;

        // 检查状态是否真的发生了变化
        if (this.ledStates[ledNum - 1] === state) {
            console.log(`[调试系统] LED${ledNum}状态已经是${state === 1 ? '开启' : '关闭'}，跳过发送`);
            return;
        }

        // 更新本地状态
        this.ledStates[ledNum - 1] = state;

        // 构建简单协议字符串
        const command = state === 1 ? `LED${ledNum}_ON` : `LED${ledNum}_OFF`;

        // 如果连接正常，立即发送；否则加入待发送队列
        if (this.isConnected && this.ws.readyState === WebSocket.OPEN) {
            this.sendCommand(command);
        } else {
            this.pendingCommands.push(command);
            console.log('[调试系统] 连接断开，命令已加入待发送队列:', command);
        }

        // 立即更新界面状态（乐观更新）
        this.updateLEDIndicator(ledNum, state);
        this.updateLEDStatus(ledNum, state);
        this.updateLEDLog(ledNum, `发送命令: ${command}`);

        console.log(`[调试系统] ${command}`);
    }

    /**
     * 发送WebSocket命令
     * @param {string} command - 要发送的命令
     */
    sendCommand(command) {
        if (this.ws && this.isConnected && this.ws.readyState === WebSocket.OPEN) {
            try {
                this.ws.send(command);
                console.log(`[调试系统] 发送命令: ${command}`);
            } catch (error) {
                console.error('[调试系统] 发送命令失败:', error);
            }
        } else {
            console.warn('[调试系统] WebSocket未连接，无法发送命令');
            // 显示连接错误提示
            this.showConnectionError();
        }
    }

    /**
     * 处理接收到的消息
     * @param {string} data - 消息数据
     */
    handleMessage(data) {
        console.log(`[调试系统] 收到消息: ${data}`);

        // 简化的响应处理，只更新状态，不触发新命令
        if (data.includes('ON OK') || data.includes('OFF OK')) {
            // 解析LED编号和状态，但不要触发新的控制
            const match = data.match(/LED(\d+)_(ON|OFF) OK/);
            if (match) {
                const ledNum = parseInt(match[1]);
                const isOn = match[2] === 'ON';

                // 只更新界面状态显示，不修改本地状态（避免循环）
                if (ledNum >= 1 && ledNum <= 6) {
                    this.updateLEDStatus(ledNum, isOn ? '开启' : '关闭');
                    this.updateLEDLog(ledNum, `收到确认: ${data}`);
                }
            }
        }
        // 其他消息忽略
    }

    /**
     * 更新LED指示灯
     * @param {number} ledNum - LED编号
     * @param {number} state - 状态
     */
    updateLEDIndicator(ledNum, state) {
        const indicator = document.getElementById(`led${ledNum}-indicator`);
        if (indicator) {
            indicator.className = `led-indicator ${state === 1 ? 'on' : 'off'}`;
        }
    }

    /**
     * 更新LED状态文本
     * @param {number} ledNum - LED编号
     * @param {number} state - 状态
     */
    updateLEDStatus(ledNum, state) {
        const statusElement = document.getElementById(`led${ledNum}-status`);
        if (statusElement) {
            statusElement.textContent = `当前状态：${state === 1 ? '开启' : '关闭'}`;
        }
    }

    /**
     * 更新LED操作日志
     * @param {number} ledNum - LED编号
     * @param {string} message - 日志消息
     */
    updateLEDLog(ledNum, message) {
        const logElement = document.getElementById(`led${ledNum}-log`);
        if (logElement) {
            const timestamp = new Date().toLocaleTimeString();
            logElement.textContent = `[${timestamp}] ${message}`;
        }
    }

    /**
     * 更新所有LED指示灯
     */
    updateAllLEDIndicators() {
        for (let i = 1; i <= 6; i++) {
            this.updateLEDIndicator(i, this.ledStates[i - 1]);
            this.updateLEDStatus(i, this.ledStates[i - 1]);
        }
    }

    /**
     * 更新连接状态显示
     * @param {boolean} connected - 连接状态
     */
    updateConnectionStatus(connected) {
        const statusDot = document.getElementById('connection-status');
        const statusText = document.getElementById('connection-text');

        if (statusDot) {
            statusDot.className = `status-dot ${connected ? '' : 'disconnected'}`;
        }

        if (statusText) {
            statusText.textContent = connected ? '连接正常' : '连接断开';
        }
    }

    /**
     * 显示连接错误提示
     */
    showConnectionError() {
        // 在每个LED的日志区域显示连接错误
        for (let i = 1; i <= 6; i++) {
            this.updateLEDLog(i, '错误：WebSocket连接断开');
        }
    }

    /**
     * 启动自动重连
     */
    startReconnect() {
        // 清除已存在的重连定时器
        if (this.reconnectInterval) {
            clearInterval(this.reconnectInterval);
        }

        // 每3秒尝试重连
        this.reconnectInterval = setInterval(() => {
            console.log('[调试系统] 尝试重新连接WebSocket...');
            this.initWebSocket();
        }, 3000);
    }

    /**
     * 停止自动重连
     */
    stopReconnect() {
        if (this.reconnectInterval) {
            clearInterval(this.reconnectInterval);
            this.reconnectInterval = null;
        }
    }

    /**
     * 发送待发送队列中的命令
     */
    sendPendingCommands() {
        if (this.pendingCommands.length > 0) {
            console.log(`[调试系统] 发送待发送队列中的 ${this.pendingCommands.length} 个命令`);

            this.pendingCommands.forEach(command => {
                this.sendCommand(command);
            });

            this.pendingCommands = []; // 清空队列
        }
    }

    /**
     * 批量控制所有LED
     * @param {number} state - 状态 (0=全部关闭, 1=全部开启)
     */
    controlAllLEDs(state) {
        console.log(`[调试系统] 批量控制所有LED: ${state === 1 ? '开启' : '关闭'}`);

        for (let i = 1; i <= 6; i++) {
            setTimeout(() => {
                this.controlLED(i, state);
            }, i * 100); // 逐个控制，间隔100ms
        }
    }

    /**
     * 获取所有LED状态
     * @returns {Array} LED状态数组
     */
    getAllLEDStates() {
        return [...this.ledStates];
    }

    /**
     * 重置所有LED状态
     */
    resetAllLEDs() {
        console.log('[调试系统] 重置所有LED状态');
        this.controlAllLEDs(0);
    }
}

/**
 * 全局函数：控制LED（供HTML调用）
 */
function controlLED(ledNum, state) {
    if (window.ledDebugSystem) {
        window.ledDebugSystem.controlLED(ledNum, state);
    }
}

/**
 * 全局函数：切换到正常模式
 */
function switchToNormalMode() {
    window.location.href = 'index.html';
}

/**
 * 全局函数：批量控制LED
 */
function turnOnAllLEDs() {
    if (window.ledDebugSystem) {
        window.ledDebugSystem.controlAllLEDs(1);
    }
}

function turnOffAllLEDs() {
    if (window.ledDebugSystem) {
        window.ledDebugSystem.controlAllLEDs(0);
    }
}

// 页面加载完成后初始化系统
document.addEventListener('DOMContentLoaded', function() {
    console.log('[调试系统] 页面加载完成，初始化LED调试系统');
    window.ledDebugSystem = new LEDDebugSystem();
});

// 页面卸载时清理WebSocket连接
window.addEventListener('beforeunload', function() {
    if (window.ledDebugSystem) {
        // 停止重连定时器
        window.ledDebugSystem.stopReconnect();

        // 关闭WebSocket连接
        if (window.ledDebugSystem.ws) {
            window.ledDebugSystem.ws.close();
        }
    }
});