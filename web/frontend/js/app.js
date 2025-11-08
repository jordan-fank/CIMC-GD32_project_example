/**
 * 智能终端管理系统 - 前端主逻辑
 */

class TerminalManager {
    constructor() {
        // WebSocket连接对象
        this.ws = null;

        // 统计数据
        this.stats = {
            sentCount: 0,
            receivedCount: 0,
            connectedAt: null,
            uptimeTimer: null
        };

        // DOM元素引用
        this.elements = {
            serverHost: document.getElementById('serverHost'),
            serverPort: document.getElementById('serverPort'),
            connectBtn: document.getElementById('connectBtn'),
            disconnectBtn: document.getElementById('disconnectBtn'),
            messageInput: document.getElementById('messageInput'),
            sendBtn: document.getElementById('sendBtn'),
            messageLog: document.getElementById('messageLog'),
            clearLogBtn: document.getElementById('clearLogBtn'),
            connectionStatus: document.getElementById('connectionStatus'),
            sentCount: document.getElementById('sentCount'),
            receivedCount: document.getElementById('receivedCount'),
            uptime: document.getElementById('uptime'),
            themeToggle: document.getElementById('themeToggle'),
            // ADC相关元素
            adcCurrentValue: document.getElementById('adcCurrentValue'),
            adcMaxValue: document.getElementById('adcMaxValue'),
            adcMinValue: document.getElementById('adcMinValue'),
            adcAvgValue: document.getElementById('adcAvgValue'),
            clearAdcBtn: document.getElementById('clearAdcBtn'),
            adcChart: document.getElementById('adcChart')
        };

        // ADC数据管理
        this.adcData = {
            values: [],
            timestamps: [],
            maxValues: 50, // 最多显示50个数据点
            currentVoltage: 0,
            maxVoltage: 0,
            minVoltage: 3.3,
            sumVoltage: 0,
            count: 0
        };

        // ADC图表实例
        this.adcChart = null;

        // 初始化
        this.init();
    }

    /**
     * 初始化事件监听器
     */
    init() {
        // 主题切换
        this.initTheme();
        if (this.elements.themeToggle) {
            this.elements.themeToggle.addEventListener('click', () => this.toggleTheme());
        }

        // 连接按钮
        this.elements.connectBtn.addEventListener('click', () => this.connect());

        // 断开按钮
        this.elements.disconnectBtn.addEventListener('click', () => this.disconnect());

        // 发送按钮
        this.elements.sendBtn.addEventListener('click', () => this.sendMessage());

        // 回车发送（Ctrl+Enter）
        this.elements.messageInput.addEventListener('keydown', (e) => {
            if (e.ctrlKey && e.key === 'Enter') {
                this.sendMessage();
            }
        });

        // 清空日志按钮
        this.elements.clearLogBtn.addEventListener('click', () => this.clearLog());

        // ADC清空数据按钮
        if (this.elements.clearAdcBtn) {
            this.elements.clearAdcBtn.addEventListener('click', () => this.clearAdcData());
        }

        // 初始化ADC图表
        this.initAdcChart();

        // 从本地存储恢复配置
        this.loadConfig();
    }

    /**
     * 连接到WebSocket服务器
     */
    connect() {
        const host = this.elements.serverHost.value.trim();
        const port = this.elements.serverPort.value.trim();

        // 验证输入
        if (!host) {
            this.addLog('error', '请输入服务器地址');
            return;
        }

        if (!port || isNaN(port) || port < 1 || port > 65535) {
            this.addLog('error', '请输入有效的端口号 (1-65535)');
            return;
        }

        // 构建WebSocket URL
        // 分离式架构：WebSocket在42623端口
        const wsUrl = `ws://${host}:${port}`;

        try {
            this.addLog('system', `正在连接到 ${wsUrl}...`);
            this.updateConnectionStatus('connecting', '连接中...');

            // 创建WebSocket连接
            this.ws = new WebSocket(wsUrl);

            // 连接打开事件
            this.ws.onopen = () => {
                this.onConnected();
                this.saveConfig();  // 保存成功的配置
            };

            // 接收消息事件
            this.ws.onmessage = (event) => {
                this.onMessageReceived(event.data);
            };

            // 连接关闭事件
            this.ws.onclose = (event) => {
                this.onDisconnected(event);
            };

            // 错误事件
            this.ws.onerror = (error) => {
                this.onError(error);
                // 如果是公网域名连接失败，提供备用方案
                if (host !== 'localhost' && host !== '127.0.0.1') {
                    this.addLog('error', '连接失败！如果使用移动网络，请尝试以下方案：');
                    this.addLog('tip', '方案1：连接WiFi网络后重试');
                    this.addLog('tip', '方案2：手动配置为localhost，然后通过端口转发连接');
                    this.addLog('tip', '方案3：确认FRP TCP隧道配置正确');
                }
            };

        } catch (error) {
            this.addLog('error', `连接失败: ${error.message}`);
            this.updateConnectionStatus('disconnected', '未连接');
        }
    }

    /**
     * 断开WebSocket连接
     */
    disconnect() {
        if (this.ws) {
            this.ws.close(1000, '用户主动断开');
            this.ws = null;
        }
    }

    /**
     * 连接成功回调
     */
    onConnected() {
        this.addLog('system', '连接成功');
        this.updateConnectionStatus('connected', '已连接');

        // 更新UI状态
        this.elements.connectBtn.disabled = true;
        this.elements.disconnectBtn.disabled = false;
        this.elements.sendBtn.disabled = false;
        this.elements.serverHost.disabled = true;
        this.elements.serverPort.disabled = true;

        // 重置统计
        this.stats.sentCount = 0;
        this.stats.receivedCount = 0;
        this.stats.connectedAt = Date.now();
        this.updateStats();

        // 启动运行时间计时器
        this.startUptimeTimer();
    }

    /**
     * 连接断开回调
     */
    onDisconnected(event) {
        const reason = event.reason || '未知原因';
        this.addLog('system', `连接已断开 (代码: ${event.code}, 原因: ${reason})`);
        this.updateConnectionStatus('disconnected', '未连接');

        // 更新UI状态
        this.elements.connectBtn.disabled = false;
        this.elements.disconnectBtn.disabled = true;
        this.elements.sendBtn.disabled = true;
        this.elements.serverHost.disabled = false;
        this.elements.serverPort.disabled = false;

        // 停止运行时间计时器
        this.stopUptimeTimer();
    }

    /**
     * 错误回调
     */
    onError(error) {
        console.error('WebSocket错误:', error);
        this.addLog('error', '连接错误，请检查服务器地址和端口');
    }

    /**
     * 接收到消息回调
     */
    onMessageReceived(data) {
        this.stats.receivedCount++;
        this.updateStats();

        // 处理 Blob 对象
        if (data instanceof Blob) {
            const reader = new FileReader();
            reader.onload = () => {
                this.processMessage(reader.result);
            };
            reader.onerror = () => {
                this.addLog('error', '读取消息失败');
            };
            reader.readAsText(data);
        } else {
            // 直接处理文本数据
            this.processMessage(data);
        }
    }

    /**
     * 处理接收到的消息文本
     */
    processMessage(text) {
        try {
            // 检查是否为ADC数据 (格式: ADC:2.35)
            if (text.startsWith('ADC:')) {
                const voltageStr = text.substring(4).trim();
                const voltage = parseFloat(voltageStr);

                if (!isNaN(voltage)) {
                    // 处理ADC数据
                    this.handleAdcData(voltage);
                    // 同时在日志中显示ADC数据
                    this.addLog('info', `ADC数据: ${voltage.toFixed(2)}V`, 'adc');
                    return;
                }
            }

            // 尝试解析JSON
            const jsonData = JSON.parse(text);

            if (jsonData.type === 'system') {
                this.addLog('system', jsonData.message);
            } else {
                this.addLog('received', JSON.stringify(jsonData, null, 2));
            }
        } catch (e) {
            // 不是JSON，直接显示原始数据
            this.addLog('received', text);
        }
    }

    /**
     * 发送消息
     */
    sendMessage() {
        const message = this.elements.messageInput.value.trim();

        if (!message) {
            this.addLog('error', '消息内容不能为空');
            return;
        }

        if (!this.ws || this.ws.readyState !== WebSocket.OPEN) {
            this.addLog('error', '未连接到服务器');
            return;
        }

        try {
            // 发送消息
            this.ws.send(message);

            // 更新统计
            this.stats.sentCount++;
            this.updateStats();

            // 添加到日志
            this.addLog('sent', message);

            // 清空输入框
            this.elements.messageInput.value = '';
            this.elements.messageInput.focus();

        } catch (error) {
            this.addLog('error', `发送失败: ${error.message}`);
        }
    }

    /**
     * 添加日志条目
     * @param {string} type - 日志类型: 'sent', 'received', 'system', 'error'
     * @param {string} message - 日志消息
     */
    addLog(type, message) {
        const logEntry = document.createElement('div');
        logEntry.className = `log-entry ${type}`;

        const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false });
        const typeLabel = {
            'sent': '发送',
            'received': '接收',
            'system': '系统',
            'error': '错误'
        }[type] || type;

        logEntry.innerHTML = `
            <span class="log-timestamp">[${timestamp}]</span>
            <span class="log-type">${typeLabel}</span>
            <span class="log-message">${this.escapeHtml(message)}</span>
        `;

        this.elements.messageLog.appendChild(logEntry);

        // 隐藏空状态
        this.toggleEmptyState();

        // 自动滚动到底部
        this.elements.messageLog.scrollTop = this.elements.messageLog.scrollHeight;
    }

    /**
     * 清空日志
     */
    clearLog() {
        // 清空所有日志条目
        const logEntries = this.elements.messageLog.querySelectorAll('.log-entry');
        logEntries.forEach(entry => entry.remove());

        // 显示空状态
        this.toggleEmptyState();
    }

    /**
     * 更新连接状态显示
     * @param {string} status - 状态: 'connected', 'disconnected', 'connecting'
     * @param {string} text - 状态文本
     */
    updateConnectionStatus(status, text) {
        const statusIndicator = this.elements.connectionStatus.querySelector('.status-indicator');
        const statusLabel = this.elements.connectionStatus.querySelector('.status-label');

        if (statusIndicator) {
            statusIndicator.className = `status-indicator ${status}`;
        }
        if (statusLabel) {
            statusLabel.textContent = text;
        }
    }

    /**
     * 更新统计信息显示
     */
    updateStats() {
        this.elements.sentCount.textContent = this.stats.sentCount;
        this.elements.receivedCount.textContent = this.stats.receivedCount;
    }

    /**
     * 启动运行时间计时器
     */
    startUptimeTimer() {
        this.stopUptimeTimer();  // 先停止之前的计时器

        this.stats.uptimeTimer = setInterval(() => {
            if (this.stats.connectedAt) {
                const uptime = Date.now() - this.stats.connectedAt;
                const hours = Math.floor(uptime / 3600000);
                const minutes = Math.floor((uptime % 3600000) / 60000);
                const seconds = Math.floor((uptime % 60000) / 1000);

                this.elements.uptime.textContent =
                    `${String(hours).padStart(2, '0')}:${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
            }
        }, 1000);
    }

    /**
     * 停止运行时间计时器
     */
    stopUptimeTimer() {
        if (this.stats.uptimeTimer) {
            clearInterval(this.stats.uptimeTimer);
            this.stats.uptimeTimer = null;
        }
        this.elements.uptime.textContent = '00:00:00';
    }

    /**
     * 保存配置到本地存储
     */
    saveConfig() {
        const config = {
            host: this.elements.serverHost.value,
            port: this.elements.serverPort.value
        };
        localStorage.setItem('terminalManagerConfig', JSON.stringify(config));
    }

    /**
     * 从本地存储加载配置
     */
    loadConfig() {
        try {
            const config = localStorage.getItem('terminalManagerConfig');
            if (config) {
                const { host, port } = JSON.parse(config);
                if (host) this.elements.serverHost.value = host;
                if (port) this.elements.serverPort.value = port;
            } else {
                // 首次访问：自动使用当前访问的域名和端口
                this.autoDetectServer();
            }
        } catch (error) {
            console.error('加载配置失败:', error);
            this.autoDetectServer();
        }
    }

    /**
     * 自动检测服务器地址（使用当前访问的域名）
     */
    autoDetectServer() {
        const currentHost = window.location.hostname;
        if (currentHost && currentHost !== 'localhost' && currentHost !== '127.0.0.1') {
            this.elements.serverHost.value = 'hk-4.lcf.im'; // FRP TCP隧道地址
            this.elements.serverPort.value = '47315'; // TCP隧道端口
            console.log(`[自动配置] 检测到域名访问: hk-4.lcf.im:47315`);
        }
    }

    /**
     * HTML转义（防止XSS）
     */
    escapeHtml(text) {
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    /**
     * 初始化主题
     */
    initTheme() {
        // 从本地存储加载主题设置
        const savedTheme = localStorage.getItem('theme') || 'light';
        this.setTheme(savedTheme);
    }

    /**
     * 切换主题
     */
    toggleTheme() {
        const currentTheme = document.documentElement.getAttribute('data-theme') || 'light';
        const newTheme = currentTheme === 'light' ? 'dark' : 'light';
        this.setTheme(newTheme);
    }

    /**
     * 设置主题
     */
    setTheme(theme) {
        document.documentElement.setAttribute('data-theme', theme);
        localStorage.setItem('theme', theme);
    }

    /**
     * 切换到调试模式
     */
    switchToDebugMode() {
        window.location.href = 'debug.html';
    }

    /**
     * 显示/隐藏空状态
     */
    toggleEmptyState() {
        const emptyState = this.elements.messageLog.querySelector('.empty-state');
        const hasMessages = this.elements.messageLog.querySelectorAll('.log-entry').length > 0;

        if (emptyState) {
            emptyState.style.display = hasMessages ? 'none' : 'flex';
        }
    }

    // ========================================
    // ADC数据处理相关方法
    // ========================================

    /**
     * 初始化ADC图表
     */
    initAdcChart() {
        if (!this.elements.adcChart) return;

        const ctx = this.elements.adcChart.getContext('2d');

        this.adcChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'ADC电压 (V)',
                    data: [],
                    borderColor: '#007AFF',
                    backgroundColor: 'rgba(0, 122, 255, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.4,
                    pointRadius: 3,
                    pointHoverRadius: 5,
                    pointBackgroundColor: '#007AFF',
                    pointBorderColor: '#FFFFFF',
                    pointBorderWidth: 2
                }]
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: {
                    duration: 200
                },
                scales: {
                    x: {
                        display: true,
                        title: {
                            display: true,
                            text: '时间',
                            color: '#8E8E93',
                            font: {
                                size: 12
                            }
                        },
                        grid: {
                            display: false
                        },
                        ticks: {
                            maxTicksLimit: 10,
                            color: '#8E8E93'
                        }
                    },
                    y: {
                        display: true,
                        title: {
                            display: true,
                            text: '电压 (V)',
                            color: '#8E8E93',
                            font: {
                                size: 12
                            }
                        },
                        min: 0,
                        max: 3.3,
                        grid: {
                            color: 'rgba(0, 0, 0, 0.05)'
                        },
                        ticks: {
                            stepSize: 0.5,
                            color: '#8E8E93',
                            callback: function(value) {
                                return value.toFixed(1);
                            }
                        }
                    }
                },
                plugins: {
                    legend: {
                        display: true,
                        position: 'top',
                        labels: {
                            color: '#3C3C43',
                            font: {
                                size: 12,
                                weight: '500'
                            },
                            usePointStyle: true,
                            boxWidth: 6
                        }
                    },
                    tooltip: {
                        backgroundColor: 'rgba(0, 0, 0, 0.8)',
                        titleColor: '#FFFFFF',
                        bodyColor: '#FFFFFF',
                        titleFont: {
                            size: 12,
                            weight: '600'
                        },
                        bodyFont: {
                            size: 12
                        },
                        padding: 12,
                        cornerRadius: 8,
                        displayColors: false,
                        callbacks: {
                            label: function(context) {
                                return `电压: ${context.parsed.y.toFixed(2)}V`;
                            }
                        }
                    }
                },
                interaction: {
                    intersect: false,
                    mode: 'index'
                }
            }
        });
    }

    /**
     * 处理ADC数据更新
     * @param {number} voltage - ADC电压值
     */
    handleAdcData(voltage) {
        // 更新当前电压
        this.adcData.currentVoltage = voltage;
        this.adcData.count++;
        this.adcData.sumVoltage += voltage;

        // 更新最大最小值
        if (voltage > this.adcData.maxVoltage) {
            this.adcData.maxVoltage = voltage;
        }
        if (voltage < this.adcData.minVoltage) {
            this.adcData.minVoltage = voltage;
        }

        // 添加到数据数组
        const now = new Date();
        const timeString = now.toLocaleTimeString('zh-CN', {
            hour12: false,
            hour: '2-digit',
            minute: '2-digit',
            second: '2-digit'
        });

        this.adcData.values.push(voltage);
        this.adcData.timestamps.push(timeString);

        // 限制数据点数量
        if (this.adcData.values.length > this.adcData.maxValues) {
            this.adcData.values.shift();
            this.adcData.timestamps.shift();
        }

        // 更新显示
        this.updateAdcDisplay();
        this.updateAdcChart();
    }

    /**
     * 更新ADC数据显示
     */
    updateAdcDisplay() {
        if (!this.elements.adcCurrentValue) return;

        // 更新当前值
        this.elements.adcCurrentValue.textContent = this.adcData.currentVoltage.toFixed(2);

        // 更新统计值
        if (this.elements.adcMaxValue) {
            this.elements.adcMaxValue.textContent = this.adcData.maxVoltage.toFixed(2);
        }
        if (this.elements.adcMinValue) {
            this.elements.adcMinValue.textContent = this.adcData.minVoltage.toFixed(2);
        }
        if (this.elements.adcAvgValue && this.adcData.count > 0) {
            const avgVoltage = this.adcData.sumVoltage / this.adcData.count;
            this.elements.adcAvgValue.textContent = avgVoltage.toFixed(2);
        }

        // 根据电压值设置颜色
        const voltageElement = this.elements.adcCurrentValue;
        voltageElement.classList.remove('high', 'medium', 'low');

        if (this.adcData.currentVoltage > 2.5) {
            voltageElement.classList.add('high');
        } else if (this.adcData.currentVoltage > 1.5) {
            voltageElement.classList.add('medium');
        } else {
            voltageElement.classList.add('low');
        }
    }

    /**
     * 更新ADC图表
     */
    updateAdcChart() {
        if (!this.adcChart) return;

        this.adcChart.data.labels = this.adcData.timestamps;
        this.adcChart.data.datasets[0].data = this.adcData.values;
        this.adcChart.update('none'); // 无动画更新，提高性能
    }

    /**
     * 清空ADC数据
     */
    clearAdcData() {
        this.adcData = {
            values: [],
            timestamps: [],
            maxValues: 50,
            currentVoltage: 0,
            maxVoltage: 0,
            minVoltage: 3.3,
            sumVoltage: 0,
            count: 0
        };

        // 重置显示
        if (this.elements.adcCurrentValue) {
            this.elements.adcCurrentValue.textContent = '0.00';
        }
        if (this.elements.adcMaxValue) {
            this.elements.adcMaxValue.textContent = '0.00';
        }
        if (this.elements.adcMinValue) {
            this.elements.adcMinValue.textContent = '0.00';
        }
        if (this.elements.adcAvgValue) {
            this.elements.adcAvgValue.textContent = '0.00';
        }

        // 清空图表
        this.updateAdcChart();

        // 添加清空日志
        this.addLogEntry('系统', 'ADC数据已清空', 'system');
    }
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', () => {
    window.terminalManager = new TerminalManager();
    console.log('智能终端管理系统已初始化');
});

/**
 * 全局函数：切换到调试模式（供HTML调用）
 */
function switchToDebugMode() {
    window.location.href = 'debug.html';
}
