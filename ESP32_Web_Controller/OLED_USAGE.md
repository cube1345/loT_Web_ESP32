# ESP32 OLED 文本显示 - 使用指南

## 功能说明

已成功实现前端输入文本 → ESP32接收 → OLED显示的完整功能！

## 已完成的修改

### 1. OLED驱动层 ([oled_integration.c](../HTTP_Demo/main/oled/oled_integration.c))
添加了新函数 `oled_show_custom_text()`:
- 在OLED顶部显示"Web Message:"标题
- 自动换行显示用户输入的文本
- 线程安全，支持并发访问

### 2. HTTP服务器 ([main.c](../HTTP_Demo/main/main.c))
添加了新的API端点 `/api/oled`:

**API端点：** `GET /api/oled`

**参数：**
- `text=<内容>` - 在OLED上显示的文本（URL编码）
- `action=clear` - 清除OLED显示

**示例请求：**
```
GET https://192.168.1.100/api/oled?text=Hello%20World
GET https://192.168.1.100/api/oled?action=clear
```

**响应：**
```json
{
  "status": "ok",
  "message": "Text displayed on OLED"
}
```

### 3. 前端控制面板
已经内置完整的OLED控制功能：
- 文本输入框
- "发送到OLED"按钮
- "清除显示"按钮
- 自动URL编码处理

## 使用步骤

### 1. 编译并烧录ESP32代码

```bash
cd e:\EmbeddedDevelopment\WorkSpace\ESPWROOM32\HTTP_Demo
idf.py build
idf.py flash
idf.py monitor
```

### 2. 查看ESP32的IP地址

在串口监视器中查看ESP32连接WiFi后获取的IP地址，例如：
```
I (1234) example: Connected to AP, IP: 192.168.1.100
```

### 3. 打开前端控制面板

双击打开 `ESP32_Web_Controller/index.html`

### 4. 连接到ESP32

1. 在"连接设置"输入ESP32的IP地址
2. 端口设为 `443` (HTTPS) 或 `80` (HTTP)
3. 勾选"使用HTTPS"
4. 点击"连接ESP32"

### 5. 发送文本到OLED

1. 在"OLED显示"卡片中的文本框输入内容，例如：
   - `Hello ESP32!`
   - `温度: 25.6°C`
   - `欢迎使用OLED显示`
   - 支持中文（如果字体库支持）

2. 点击"发送到OLED"按钮

3. OLED屏幕会显示：
   ```
   Web Message:
   您输入的文本内容
   （自动换行）
   ```

## OLED显示特性

- **分辨率：** 128x64 像素
- **接口：** I2C (SDA=GPIO25, SCL=GPIO26)
- **地址：** 0x3C
- **刷新速率限制：** 最小100ms间隔（防止I2C总线过载）
- **文本模式：** 自动换行显示长文本
- **线程安全：** 使用互斥锁保护并发访问

## 测试示例

### 测试1：简单文本
输入：`Hello World!`
OLED显示：
```
Web Message:
Hello World!
```

### 测试2：长文本（自动换行）
输入：`This is a very long message that will automatically wrap to multiple lines on the OLED display`
OLED显示：
```
Web Message:
This is a very
long message that
will automatically
wrap to multiple...
```

### 测试3：清除显示
点击"清除显示"按钮，OLED会被清空

## 故障排除

### 问题1：连接失败
- 确认ESP32和电脑在同一网络
- 检查IP地址是否正确
- 如使用HTTPS，需先在浏览器中接受证书

### 问题2：OLED无显示
- 检查OLED硬件连接：
  - SDA → GPIO 25
  - SCL → GPIO 26
  - VCC → 3.3V
  - GND → GND
- 查看ESP32串口日志确认OLED初始化成功
- 确认I2C地址为0x3C

### 问题3：中文乱码
- 当前使用的字体库可能不支持中文
- 建议使用英文和数字进行测试

## 日志输出

ESP32端会输出日志：
```
I (12345) oled_integration: Displayed on OLED: Hello World!
I (12346) example: Received text for OLED: Hello World!
```

前端会显示：
```
[14:23:45] 发送 GET 请求到: /api/oled?text=Hello%20World
[14:23:45] 请求成功 (45ms): {"status":"ok","message":"Text displayed on OLED"}
```

## 扩展功能建议

可以进一步扩展的功能：
1. 支持多行文本独立控制
2. 添加字体大小选择
3. 支持图标和图形显示
4. 添加屏幕亮度控制
5. 实现文本滚动效果

## API完整示例

### 使用curl测试
```bash
# 显示文本
curl -k "https://192.168.1.100/api/oled?text=Hello%20ESP32"

# 清除显示
curl -k "https://192.168.1.100/api/oled?action=clear"
```

### 使用JavaScript (fetch)
```javascript
// 显示文本
fetch(`https://192.168.1.100/api/oled?text=${encodeURIComponent('Hello World')}`)
  .then(r => r.json())
  .then(data => console.log(data));

// 清除显示
fetch('https://192.168.1.100/api/oled?action=clear')
  .then(r => r.json())
  .then(data => console.log(data));
```

---

**状态：** ✅ 功能已完全实现并测试
**最后更新：** 2025-12-24
