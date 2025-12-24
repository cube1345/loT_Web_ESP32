# ESP32 Web Controller

一个用于控制ESP32设备的Web前端控制面板。

## 功能特性

- ✅ 支持 HTTP/HTTPS 连接
- ✅ LED 控制（开/关/切换）
- ✅ GPIO 引脚控制
- ✅ OLED 显示屏控制
- ✅ 获取网络笑话功能
- ✅ 自定义API端点请求
- ✅ 实时日志记录
- ✅ 响应时间监控
- ✅ 连接状态指示
- ✅ 本地配置保存

## 快速开始

### 1. 配置ESP32

确保你的ESP32设备已经：
- 连接到与电脑相同的局域网
- 运行HTTP/HTTPS服务器
- 记录下ESP32的IP地址

### 2. 使用前端控制面板

#### 方法一：直接打开HTML文件
```bash
# 双击 index.html 文件即可在浏览器中打开
```

#### 方法二：使用本地HTTP服务器（推荐）
```bash
# 使用Python启动简单服务器
cd ESP32_Web_Controller
python -m http.server 8080

# 或使用Node.js
npx http-server -p 8080
```

然后在浏览器访问：`http://localhost:8080`

### 3. 连接到ESP32

1. 在"连接设置"中输入ESP32的IP地址（例如：192.168.1.100）
2. 选择端口（默认HTTPS是443，HTTP是80）
3. 勾选或取消"使用HTTPS"
4. 点击"连接ESP32"按钮

## HTTPS证书问题

如果使用HTTPS连接，ESP32使用的是自签名证书，浏览器会显示安全警告。

**解决方法：**

1. 首先在浏览器中直接访问ESP32的HTTPS地址（如：`https://192.168.1.100`）
2. 浏览器会显示证书警告，点击"高级"或"详细信息"
3. 选择"继续访问"或"接受风险并继续"
4. 之后再使用控制面板连接即可

## ESP32 API端点配置

当前前端支持以下API端点（你需要在ESP32代码中实现）：

### 基础端点
- `GET /` - 首页，用于连接测试

### LED控制
- `GET /api/led?action=on` - 开启LED
- `GET /api/led?action=off` - 关闭LED
- `GET /api/led?action=toggle` - 切换LED状态

### GPIO控制
- `GET /api/gpio?pin=<PIN>&level=high` - 设置GPIO高电平
- `GET /api/gpio?pin=<PIN>&level=low` - 设置GPIO低电平

### OLED控制
- `GET /api/oled?text=<TEXT>` - 在OLED上显示文本
- `GET /api/oled?action=clear` - 清除OLED显示

### 笑话功能
- `GET /api/joke` - 触发获取并显示笑话

## 在ESP32上添加API端点

在你的 `main.c` 文件中添加以下处理器：

```c
// LED控制处理器
static esp_err_t led_handler(httpd_req_t *req)
{
    char query[128];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char action[16];
        if (httpd_query_key_value(query, "action", action, sizeof(action)) == ESP_OK) {
            if (strcmp(action, "on") == 0) {
                // 开启LED代码
                gpio_set_level(LED_PIN, 1);
                httpd_resp_send(req, "LED ON", HTTPD_RESP_USE_STRLEN);
            } else if (strcmp(action, "off") == 0) {
                // 关闭LED代码
                gpio_set_level(LED_PIN, 0);
                httpd_resp_send(req, "LED OFF", HTTPD_RESP_USE_STRLEN);
            }
        }
    }
    return ESP_OK;
}

// GPIO控制处理器
static esp_err_t gpio_handler(httpd_req_t *req)
{
    char query[128];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char pin_str[8], level[8];
        if (httpd_query_key_value(query, "pin", pin_str, sizeof(pin_str)) == ESP_OK &&
            httpd_query_key_value(query, "level", level, sizeof(level)) == ESP_OK) {
            int pin = atoi(pin_str);
            gpio_set_direction(pin, GPIO_MODE_OUTPUT);
            gpio_set_level(pin, strcmp(level, "high") == 0 ? 1 : 0);
            httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        }
    }
    return ESP_OK;
}

// OLED显示处理器
static esp_err_t oled_handler(httpd_req_t *req)
{
    char query[256];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char text[128];
        if (httpd_query_key_value(query, "text", text, sizeof(text)) == ESP_OK) {
            // 显示文本到OLED
            oled_show_text(text);
            httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
        }
    }
    return ESP_OK;
}

// 注册URI处理器
static const httpd_uri_t led_uri = {
    .uri = "/api/led",
    .method = HTTP_GET,
    .handler = led_handler
};

static const httpd_uri_t gpio_uri = {
    .uri = "/api/gpio",
    .method = HTTP_GET,
    .handler = gpio_handler
};

static const httpd_uri_t oled_uri = {
    .uri = "/api/oled",
    .method = HTTP_GET,
    .handler = oled_handler
};

// 在 start_webserver() 中注册
httpd_register_uri_handler(server, &led_uri);
httpd_register_uri_handler(server, &gpio_uri);
httpd_register_uri_handler(server, &oled_uri);
```

## 项目结构

```
ESP32_Web_Controller/
├── index.html      # 主页面
├── styles.css      # 样式文件
├── app.js          # 控制逻辑
└── README.md       # 说明文档
```

## 键盘快捷键

- `Ctrl + Enter` / `Cmd + Enter` - 快速连接ESP32
- `Ctrl + L` / `Cmd + L` - 清除日志

## 技术栈

- 纯原生 HTML5 + CSS3 + JavaScript
- 无需任何框架或依赖
- 支持所有现代浏览器

## 浏览器兼容性

- ✅ Chrome/Edge 90+
- ✅ Firefox 88+
- ✅ Safari 14+
- ✅ Opera 76+

## 故障排除

### 无法连接到ESP32
1. 确认ESP32和电脑在同一网络
2. 检查ESP32的IP地址是否正确
3. 确认防火墙未阻止连接
4. 尝试使用HTTP而非HTTPS

### HTTPS证书错误
1. 直接在浏览器访问ESP32的HTTPS地址
2. 手动接受证书警告
3. 或在ESP32上配置有效的SSL证书

### 跨域问题
如果使用本地文件打开，某些浏览器可能有跨域限制。建议使用本地HTTP服务器。

## 自定义扩展

你可以轻松添加新的控制功能：

1. 在HTML中添加控制按钮
2. 在JavaScript中添加对应的函数
3. 在ESP32中添加相应的API端点

## 许可证

MIT License

## 作者

ESP32 Web Controller
2025-12-24
