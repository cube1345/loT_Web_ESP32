// ESP32 Web Controller JavaScript

class ESP32Controller {
    constructor() {
        this.baseUrl = '';
        this.isConnected = false;
        this.useHttps = true;
        this.init();
    }

    init() {
        // 初始化事件监听器
        document.getElementById('connectBtn').addEventListener('click', () => this.connect());
        
        // 加载保存的配置
        this.loadConfig();
        
        // 添加日志
        this.log('系统初始化完成 - 默认使用HTTP (无需证书)', 'info');
    }

    // 加载保存的配置
    loadConfig() {
        const savedIp = localStorage.getItem('esp32Ip');
        const savedPort = localStorage.getItem('esp32Port');
        const savedHttps = localStorage.getItem('useHttps');
        
        if (savedIp) document.getElementById('esp32Ip').value = savedIp;
        if (savedPort) document.getElementById('esp32Port').value = savedPort;
        if (savedHttps !== null) {
            document.getElementById('useHttps').checked = savedHttps === 'true';
        }
    }

    // 保存配置
    saveConfig() {
        const ip = document.getElementById('esp32Ip').value;
        const port = document.getElementById('esp32Port').value;
        const https = document.getElementById('useHttps').checked;
        
        localStorage.setItem('esp32Ip', ip);
        localStorage.setItem('esp32Port', port);
        localStorage.setItem('useHttps', https);
    }

    // 连接ESP32
    async connect() {
        const ip = document.getElementById('esp32Ip').value.trim();
        const port = document.getElementById('esp32Port').value.trim();
        this.useHttps = document.getElementById('useHttps').checked;
        
        if (!ip) {
            this.log('请输入ESP32的IP地址', 'error');
            return;
        }

        const protocol = this.useHttps ? 'https' : 'http';
        this.baseUrl = `${protocol}://${ip}:${port}`;
        
        // 清晰显示使用的协议
        const protocolInfo = this.useHttps ? 'HTTPS (需要证书)' : 'HTTP (无需证书)';
        this.log(`使用协议: ${protocolInfo}`, 'info');
        this.log(`正在连接到 ${this.baseUrl}...`, 'info');
        
        try {
            const startTime = Date.now();
            const response = await this.fetchWithTimeout(this.baseUrl + '/');
            const responseTime = Date.now() - startTime;
            
            if (response.ok) {
                this.isConnected = true;
                this.updateConnectionStatus(true);
                this.log(`连接成功！响应时间: ${responseTime}ms`, 'success');
                this.saveConfig();
                
                // 更新设备信息
                document.getElementById('deviceStatus').textContent = '在线';
                document.getElementById('responseTime').textContent = `${responseTime}ms`;
            } else {
                throw new Error(`HTTP ${response.status}`);
            }
        } catch (error) {
            this.isConnected = false;
            this.updateConnectionStatus(false);
            this.log(`连接失败: ${error.message}`, 'error');
            
            if (this.useHttps) {
                this.log('提示: 如果使用自签名证书，请在浏览器中信任该证书', 'warning');
            }
        }
    }

    // 更新连接状态显示
    updateConnectionStatus(connected) {
        const statusIndicator = document.getElementById('statusIndicator');
        const statusText = document.getElementById('statusText');
        
        if (connected) {
            statusIndicator.className = 'status-dot online';
            statusText.textContent = '已连接';
            document.getElementById('connectBtn').textContent = '重新连接';
            document.getElementById('connectBtn').className = 'btn btn-success';
        } else {
            statusIndicator.className = 'status-dot offline';
            statusText.textContent = '未连接';
            document.getElementById('connectBtn').textContent = '连接 ESP32';
            document.getElementById('connectBtn').className = 'btn btn-primary';
            document.getElementById('deviceStatus').textContent = '-';
            document.getElementById('responseTime').textContent = '-';
        }
    }

    // 带超时的fetch请求
    async fetchWithTimeout(url, options = {}, timeout = 5000) {
        const controller = new AbortController();
        const timeoutId = setTimeout(() => controller.abort(), timeout);
        
        try {
            const response = await fetch(url, {
                ...options,
                signal: controller.signal
            });
            clearTimeout(timeoutId);
            return response;
        } catch (error) {
            clearTimeout(timeoutId);
            throw error;
        }
    }

    // 发送命令到ESP32
    async sendRequest(endpoint, method = 'GET', body = null) {
        if (!this.isConnected) {
            this.log('请先连接到ESP32设备', 'warning');
            return null;
        }

        const url = this.baseUrl + endpoint;
        this.log(`发送 ${method} 请求到: ${endpoint}`, 'info');

        try {
            const options = {
                method: method,
                headers: {
                    'Content-Type': 'application/json',
                }
            };

            if (body) {
                options.body = JSON.stringify(body);
            }

            const startTime = Date.now();
            const response = await this.fetchWithTimeout(url, options);
            const responseTime = Date.now() - startTime;

            if (response.ok) {
                const text = await response.text();
                this.log(`请求成功 (${responseTime}ms): ${text.substring(0, 100)}`, 'success');
                document.getElementById('responseTime').textContent = `${responseTime}ms`;
                return { success: true, data: text };
            } else {
                throw new Error(`HTTP ${response.status}`);
            }
        } catch (error) {
            this.log(`请求失败: ${error.message}`, 'error');
            return { success: false, error: error.message };
        }
    }

    // 添加日志
    log(message, type = 'info') {
        const logContainer = document.getElementById('logContainer');
        const timestamp = new Date().toLocaleTimeString();
        const logEntry = document.createElement('p');
        logEntry.className = `log-entry ${type}`;
        logEntry.innerHTML = `<span class="log-timestamp">[${timestamp}]</span>${message}`;
        
        logContainer.appendChild(logEntry);
        logContainer.scrollTop = logContainer.scrollHeight;

        // 限制日志条数
        const maxLogs = 100;
        while (logContainer.children.length > maxLogs) {
            logContainer.removeChild(logContainer.firstChild);
        }
    }
}

// 创建全局控制器实例
const controller = new ESP32Controller();

// 全局函数供HTML调用

function sendCommand(type, action) {
    let endpoint = '/';
    
    switch(type) {
        case 'led':
            endpoint = `/api/led?action=${action}`;
            break;
        case 'joke':
            endpoint = '/api/joke';
            break;
        case 'oled':
            endpoint = `/api/oled?action=${action}`;
            break;
    }
    
    controller.sendRequest(endpoint);
}

function setGPIO(level) {
    const pin = document.getElementById('gpioPin').value;
    if (!pin) {
        controller.log('请输入GPIO引脚号', 'warning');
        return;
    }
    
    const endpoint = `/api/gpio?pin=${pin}&level=${level}`;
    controller.sendRequest(endpoint);
}

function sendToOLED() {
    const text = document.getElementById('oledText').value;
    if (!text) {
        controller.log('请输入要显示的文本', 'warning');
        return;
    }
    
    const endpoint = `/api/oled?text=${encodeURIComponent(text)}`;
    controller.sendRequest(endpoint);
}

function sendCustomCommand() {
    const endpoint = document.getElementById('customEndpoint').value;
    const method = document.getElementById('customMethod').value;
    
    if (!endpoint) {
        controller.log('请输入端点路径', 'warning');
        return;
    }
    
    controller.sendRequest(endpoint, method);
}

function clearLogs() {
    const logContainer = document.getElementById('logContainer');
    logContainer.innerHTML = '<p class="log-entry info">日志已清除</p>';
}

// 键盘快捷键
document.addEventListener('keydown', (e) => {
    // Ctrl/Cmd + Enter: 连接
    if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
        controller.connect();
    }
    
    // Ctrl/Cmd + L: 清除日志
    if ((e.ctrlKey || e.metaKey) && e.key === 'l') {
        e.preventDefault();
        clearLogs();
    }
});

// 页面加载完成提示
    controller.log('默认配置: HTTP协议 (80端口)，无需证书', 'info');
    
    // 清除可能的旧配置
    const useHttps = document.getElementById('useHttps').checked;
    if (useHttps) {
        controller.log('警告: 当前勾选了HTTPS，请确保ESP32支持并已信任证书', 'warning');
    } else {
        controller.log('✓ 当前使用HTTP模式，简单直接', 'success');
    }
window.addEventListener('load', () => {
    controller.log('欢迎使用ESP32控制面板！', 'success');
    controller.log('提示: 使用 Ctrl+Enter 快速连接，Ctrl+L 清除日志', 'info');
});
