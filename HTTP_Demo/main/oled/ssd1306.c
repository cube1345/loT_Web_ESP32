#include "ssd1306.h"
#include <string.h>
#include <stdlib.h>
#include <esp_log.h>

static const char *TAG __attribute__((unused)) = "ssd1306";

/* 5x8 Monospace Font (ASCII 0x20-0x7F) */
static const uint8_t font_5x8[96][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, {0x00, 0x00, 0x5f, 0x00, 0x00}, {0x00, 0x07, 0x00, 0x07, 0x00}, {0x14, 0x7f, 0x14, 0x7f, 0x14}, {0x24, 0x2a, 0x7f, 0x2a, 0x12}, {0x23, 0x13, 0x08, 0x64, 0x62}, {0x36, 0x49, 0x55, 0x22, 0x50}, {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1c, 0x22, 0x41, 0x00}, {0x00, 0x41, 0x22, 0x1c, 0x00}, {0x14, 0x08, 0x3e, 0x08, 0x14}, {0x08, 0x08, 0x3e, 0x08, 0x08}, {0x00, 0x50, 0x30, 0x00, 0x00}, {0x08, 0x08, 0x08, 0x08, 0x08}, {0x00, 0x60, 0x60, 0x00, 0x00}, {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3e, 0x51, 0x49, 0x45, 0x3e}, {0x00, 0x42, 0x7f, 0x40, 0x00}, {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4b, 0x31}, {0x18, 0x14, 0x12, 0x7f, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39}, {0x3c, 0x4a, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1e}, {0x00, 0x36, 0x36, 0x00, 0x00}, {0x00, 0x56, 0x36, 0x00, 0x00}, {0x08, 0x14, 0x22, 0x41, 0x00}, {0x14, 0x14, 0x14, 0x14, 0x14}, {0x00, 0x41, 0x22, 0x14, 0x08}, {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3e}, {0x7e, 0x11, 0x11, 0x11, 0x7e}, {0x7f, 0x49, 0x49, 0x49, 0x36}, {0x3e, 0x41, 0x41, 0x41, 0x22}, {0x7f, 0x41, 0x41, 0x22, 0x1c}, {0x7f, 0x49, 0x49, 0x49, 0x41}, {0x7f, 0x09, 0x09, 0x09, 0x01}, {0x3e, 0x41, 0x49, 0x49, 0x7a},
    {0x7f, 0x08, 0x08, 0x08, 0x7f}, {0x00, 0x41, 0x7f, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3f, 0x01}, {0x7f, 0x08, 0x14, 0x22, 0x41}, {0x7f, 0x40, 0x40, 0x40, 0x40}, {0x7f, 0x02, 0x0c, 0x02, 0x7f}, {0x7f, 0x04, 0x08, 0x10, 0x7f}, {0x3e, 0x41, 0x41, 0x41, 0x3e},
    {0x7f, 0x09, 0x09, 0x09, 0x06}, {0x3e, 0x41, 0x51, 0x21, 0x5e}, {0x7f, 0x09, 0x19, 0x29, 0x46}, {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7f, 0x01, 0x01}, {0x3f, 0x40, 0x40, 0x40, 0x3f}, {0x1f, 0x20, 0x40, 0x20, 0x1f}, {0x3f, 0x40, 0x38, 0x40, 0x3f},
    {0x63, 0x14, 0x08, 0x14, 0x63}, {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43}, {0x00, 0x7f, 0x41, 0x41, 0x00}, {0x02, 0x04, 0x08, 0x10, 0x20}, {0x00, 0x41, 0x41, 0x7f, 0x00}, {0x04, 0x02, 0x01, 0x02, 0x04}, {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00}, {0x20, 0x54, 0x54, 0x54, 0x78}, {0x7f, 0x48, 0x44, 0x44, 0x38}, {0x38, 0x44, 0x44, 0x44, 0x20}, {0x38, 0x44, 0x44, 0x48, 0x7f}, {0x38, 0x54, 0x54, 0x54, 0x18}, {0x08, 0x7e, 0x09, 0x01, 0x02}, {0x0c, 0x52, 0x52, 0x52, 0x3e},
    {0x7f, 0x08, 0x04, 0x04, 0x78}, {0x00, 0x44, 0x7d, 0x40, 0x00}, {0x20, 0x40, 0x44, 0x3d, 0x00}, {0x7f, 0x10, 0x28, 0x44, 0x00}, {0x00, 0x41, 0x7f, 0x40, 0x00}, {0x7c, 0x04, 0x18, 0x04, 0x78}, {0x7c, 0x08, 0x04, 0x04, 0x78}, {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7c, 0x14, 0x14, 0x14, 0x08}, {0x08, 0x14, 0x14, 0x18, 0x7c}, {0x7c, 0x08, 0x04, 0x04, 0x08}, {0x48, 0x54, 0x54, 0x54, 0x20}, {0x04, 0x3f, 0x44, 0x40, 0x20}, {0x3c, 0x40, 0x40, 0x44, 0x7c}, {0x1c, 0x20, 0x40, 0x20, 0x1c}, {0x3c, 0x40, 0x30, 0x40, 0x3c},
    {0x44, 0x28, 0x10, 0x28, 0x44}, {0x0c, 0x50, 0x50, 0x50, 0x3c}, {0x44, 0x64, 0x54, 0x4c, 0x44}, {0x08, 0x36, 0x41, 0x41, 0x00}, {0x00, 0x00, 0x7f, 0x00, 0x00}, {0x00, 0x41, 0x41, 0x36, 0x08}, {0x08, 0x04, 0x08, 0x10, 0x08},
};

/* 函数名：ssd1306_write_cmd
 *
 * 函数说明：向 SSD1306 发送单条命令（I2C 控制字节 0x00）。
 * 参数：
 *   dev - 设备句柄，包含 I2C 设备信息。
 *   cmd - 要发送的命令字节。
 * 返回值：
 *   ESP_OK 表示发送成功，其他为 I2C 相关错误码。
 */
static esp_err_t ssd1306_write_cmd(ssd1306_t *dev, uint8_t cmd)
{
    uint8_t data[2] = {I2C_CMD_BYTE, cmd};
    return i2c_master_transmit(dev->i2c_dev, data, 2, 100);
}

/* 函数名：ssd1306_write_data
 *
 * 函数说明：向 SSD1306 发送数据流，自动按块分段并添加数据前缀 0x40。
 * 参数：
 *   dev - 设备句柄。
 *   buf - 数据缓冲区指针。
 *   len - 数据长度。
 * 返回值：
 *   ESP_OK 表示发送成功，其他为 I2C 相关错误码。
 */
static esp_err_t ssd1306_write_data(ssd1306_t *dev, uint8_t *buf, size_t len)
{
    /* Send data in safe chunks, each transaction prefixed with one data-control byte (0x40) */
    const size_t CHUNK = 32;  /* tune: 16~64 typically fine for I2C controller */
    size_t sent = 0;
    while (sent < len) {
        size_t n = len - sent;
        if (n > CHUNK) n = CHUNK;
        uint8_t txbuf[1 + CHUNK];
        txbuf[0] = I2C_DATA_BYTE; /* 0x40: Co=0, D/C#=1 (data stream) */
        memcpy(&txbuf[1], buf + sent, n);
        esp_err_t ret = i2c_master_transmit(dev->i2c_dev, txbuf, (size_t)(1 + n), 200);
        if (ret != ESP_OK) return ret;
        sent += n;
    }
    return ESP_OK;
}

/* 函数名：ssd1306_init_display
 *
 * 函数说明：向 SSD1306 下发初始化指令序列，并清屏显示。
 * 参数：
 *   dev - 设备句柄。
 * 返回值：
 *   ESP_OK 表示初始化成功，其他为 I2C/设备错误码。
 */
static esp_err_t ssd1306_init_display(ssd1306_t *dev)
{
    esp_err_t ret;
    uint8_t init_cmds[] = {
        SET_DISP | 0x00,
        SET_MEM_ADDR, 0x00,
        SET_DISP_START_LINE | 0x00,
        SET_SEG_REMAP | 0x01,
        SET_MUX_RATIO, dev->height - 1,
        SET_COM_OUT_DIR | 0x08,
        SET_DISP_OFFSET, 0x00,
        SET_COM_PIN_CFG, dev->height == 32 ? 0x02 : 0x12,
        SET_DISP_CLK_DIV, 0x80,
        SET_PRECHARGE, dev->external_vcc ? 0x22 : 0xf1,
        SET_VCOM_DESEL, 0x30,
        SET_CONTRAST, 0xff,
        SET_ENTIRE_ON,
        SET_NORM_INV,
        SET_CHARGE_PUMP, dev->external_vcc ? 0x10 : 0x14,
        SET_DISP | 0x01,
    };
    
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        ret = ssd1306_write_cmd(dev, init_cmds[i]);
        if (ret != ESP_OK) return ret;
    }
    
    ssd1306_fill(dev, 0);
    return ssd1306_show(dev);
}

/* 函数名：ssd1306_reset_dirty
 *
 * 函数说明：重置脏标记，清空页面与列的脏区记录。
 * 参数：
 *   dev - 设备句柄。
 * 返回值：
 *   无。
 */
static inline void ssd1306_reset_dirty(ssd1306_t *dev)
{
    dev->dirty_flags = 0;
    for (uint8_t p = 0; p < dev->pages && p < 8; ++p) {
        dev->page_dirty[p] = 0;
        dev->dirty_col_start[p] = 0xFF;
        dev->dirty_col_end[p] = 0;
    }
}

/* 函数名：ssd1306_mark_dirty
 *
 * 函数说明：标记指定像素所在页与列区间为脏，用于增量刷新。
 * 参数：
 *   dev - 设备句柄。
 *   x   - 像素 X 坐标。
 *   y   - 像素 Y 坐标。
 * 返回值：
 *   无。
 */
static inline void ssd1306_mark_dirty(ssd1306_t *dev, uint16_t x, uint16_t y)
{
    if (x >= dev->width || y >= dev->height) return;
    uint8_t page = y / 8;
    if (page >= 8) return;
    dev->dirty_flags |= 0x01;
    dev->page_dirty[page] = 1;
    if (dev->dirty_col_start[page] == 0xFF || x < dev->dirty_col_start[page]) dev->dirty_col_start[page] = (uint8_t)x;
    if (x > dev->dirty_col_end[page]) dev->dirty_col_end[page] = (uint8_t)x;
}

/* 函数名：ssd1306_init
 *
 * 函数说明：初始化 SSD1306 结构，分配帧缓冲并完成硬件初始化。
 * 参数：
 *   dev - 设备句柄指针。
 *   i2c_dev - 已创建的 I2C 设备句柄。
 *   width - 屏幕宽度（像素）。
 *   height - 屏幕高度（像素）。
 *   i2c_addr - I2C 地址。
 *   external_vcc - 是否使用外部供电。
 * 返回值：
 *   ESP_OK 表示成功，参数错误或分配/初始化失败返回对应错误码。
 */
esp_err_t ssd1306_init(ssd1306_t *dev, i2c_master_dev_handle_t i2c_dev,
                       uint16_t width, uint16_t height, uint8_t i2c_addr,
                       bool external_vcc)
{
    if (!dev || !i2c_dev) return ESP_ERR_INVALID_ARG;
    
    dev->width = width;
    dev->height = height;
    dev->pages = height / 8;
    dev->i2c_dev = i2c_dev;
    dev->i2c_addr = i2c_addr;
    dev->external_vcc = external_vcc;
    
    size_t buffer_size = dev->pages * dev->width;
    dev->buffer = (uint8_t *)malloc(buffer_size);
    if (!dev->buffer) {
        ESP_LOGE(TAG, "Failed to allocate framebuffer");
        return ESP_ERR_NO_MEM;
    }
    
    memset(dev->buffer, 0, buffer_size);
    ESP_LOGI(TAG, "Initializing SSD1306 %dx%d at 0x%02x", width, height, i2c_addr);
    ssd1306_reset_dirty(dev);
    
    return ssd1306_init_display(dev);
}

/* 函数名：ssd1306_deinit
 *
 * 函数说明：释放帧缓冲等资源，不做 I2C 反初始化。
 * 参数：
 *   dev - 设备句柄。
 * 返回值：
 *   无。
 */
void ssd1306_deinit(ssd1306_t *dev)
{
    if (dev && dev->buffer) {
        free(dev->buffer);
        dev->buffer = NULL;
    }
}

/* 函数名：ssd1306_power_on
 *
 * 函数说明：打开显示（显示开启位）。
 * 参数：
 *   dev - 设备句柄。
 * 返回值：
 *   ESP_OK 表示成功。
 */
esp_err_t ssd1306_power_on(ssd1306_t *dev)
{
    return ssd1306_write_cmd(dev, SET_DISP | 0x01);
}

/* 函数名：ssd1306_power_off
 *
 * 函数说明：关闭显示（显示关闭位）。
 * 参数：
 *   dev - 设备句柄。
 * 返回值：
 *   ESP_OK 表示成功。
 */
esp_err_t ssd1306_power_off(ssd1306_t *dev)
{
    return ssd1306_write_cmd(dev, SET_DISP | 0x00);
}

/* 函数名：ssd1306_set_contrast
 *
 * 函数说明：设置显示对比度。
 * 参数：
 *   dev - 设备句柄。
 *   contrast - 对比度值（0-255）。
 * 返回值：
 *   ESP_OK 表示成功，否则为 I2C 错误码。
 */
esp_err_t ssd1306_set_contrast(ssd1306_t *dev, uint8_t contrast)
{
    esp_err_t ret = ssd1306_write_cmd(dev, SET_CONTRAST);
    if (ret != ESP_OK) return ret;
    return ssd1306_write_cmd(dev, contrast);
}

/* 函数名：ssd1306_invert
 *
 * 函数说明：设置正显或反显模式。
 * 参数：
 *   dev - 设备句柄。
 *   invert - true 为反显，false 为正显。
 * 返回值：
 *   ESP_OK 表示成功，否则为 I2C 错误码。
 */
esp_err_t ssd1306_invert(ssd1306_t *dev, bool invert)
{
    return ssd1306_write_cmd(dev, SET_NORM_INV | (invert ? 1 : 0));
}

/* 函数名：ssd1306_show
 *
 * 函数说明：将脏矩形区域写回屏幕，实现增量刷新。
 * 参数：
 *   dev - 设备句柄。
 * 返回值：
 *   ESP_OK 表示成功，其他为 I2C 错误码。
 */
esp_err_t ssd1306_show(ssd1306_t *dev)
{
    esp_err_t ret;
    if (!(dev->dirty_flags & 0x01)) {
        return ESP_OK; /* Nothing to update */
    }

    for (uint8_t page = 0; page < dev->pages && page < 8; ++page) {
        if (!dev->page_dirty[page]) continue;

        uint8_t start_col = dev->dirty_col_start[page];
        uint8_t end_col = dev->dirty_col_end[page];
        if (start_col == 0xFF || end_col < start_col) continue;

        /* Set column address range */
        ret = ssd1306_write_cmd(dev, SET_COL_ADDR);
        if (ret != ESP_OK) return ret;
        ret = ssd1306_write_cmd(dev, start_col);
        if (ret != ESP_OK) return ret;
        ret = ssd1306_write_cmd(dev, end_col);
        if (ret != ESP_OK) return ret;

        /* Set page address to this page only */
        ret = ssd1306_write_cmd(dev, SET_PAGE_ADDR);
        if (ret != ESP_OK) return ret;
        ret = ssd1306_write_cmd(dev, page);
        if (ret != ESP_OK) return ret;
        ret = ssd1306_write_cmd(dev, page);
        if (ret != ESP_OK) return ret;

        size_t len = (size_t)(end_col - start_col + 1);
        size_t offset = (size_t)page * dev->width + start_col;
        ret = ssd1306_write_data(dev, dev->buffer + offset, len);
        if (ret != ESP_OK) return ret;

        /* Clear dirty for this page */
        dev->page_dirty[page] = 0;
        dev->dirty_col_start[page] = 0xFF;
        dev->dirty_col_end[page] = 0;
    }

    dev->dirty_flags &= ~0x01;
    return ESP_OK;
}

/* 函数名：ssd1306_fill
 *
 * 函数说明：填充整个缓冲区为指定颜色，并标记全部为脏。
 * 参数：
 *   dev - 设备句柄。
 *   color - 0 清空，非 0 填满。
 * 返回值：
 *   无。
 */
void ssd1306_fill(ssd1306_t *dev, uint8_t color)
{
    memset(dev->buffer, color ? 0xff : 0x00, dev->pages * dev->width);
    /* Mark all pages and columns as dirty */
    dev->dirty_flags |= 0x01;
    for (uint8_t p = 0; p < dev->pages && p < 8; ++p) {
        dev->page_dirty[p] = 1;
        dev->dirty_col_start[p] = 0;
        dev->dirty_col_end[p] = (uint8_t)(dev->width - 1);
    }
}

/* 函数名：ssd1306_clear
 *
 * 函数说明：清空缓冲区并标记为脏（调用 fill 置零）。
 * 参数：
 *   dev - 设备句柄。
 * 返回值：
 *   无。
 */
void ssd1306_clear(ssd1306_t *dev)
{
    ssd1306_fill(dev, 0);
}

/* 函数名：ssd1306_pixel
 *
 * 函数说明：设置单个像素并标记脏区。
 * 参数：
 *   dev - 设备句柄。
 *   x   - 像素 X 坐标。
 *   y   - 像素 Y 坐标。
 *   color - 非 0 置 1，0 置 0。
 * 返回值：
 *   无。
 */
void ssd1306_pixel(ssd1306_t *dev, uint16_t x, uint16_t y, uint8_t color)
{
    if (x >= dev->width || y >= dev->height) return;
    
    uint16_t pos = (y / 8) * dev->width + x;
    uint8_t bit = 1 << (y % 8);
    
    if (color) {
        dev->buffer[pos] |= bit;
    } else {
        dev->buffer[pos] &= ~bit;
    }

    ssd1306_mark_dirty(dev, x, y);
}

/* 函数名：ssd1306_h_line
 *
 * 函数说明：绘制水平线段。
 * 参数：
 *   dev - 设备句柄。
 *   x   - 起点 X 坐标。
 *   y   - 起点 Y 坐标。
 *   width - 线段长度。
 *   color - 非 0 置 1，0 置 0。
 * 返回值：
 *   无。
 */
void ssd1306_h_line(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t width, uint8_t color)
{
    for (uint16_t i = 0; i < width; i++) {
        ssd1306_pixel(dev, x + i, y, color);
    }
}

/* 函数名：ssd1306_v_line
 *
 * 函数说明：绘制垂直线段。
 * 参数：
 *   dev - 设备句柄。
 *   x   - 起点 X 坐标。
 *   y   - 起点 Y 坐标。
 *   height - 线段高度。
 *   color - 非 0 置 1，0 置 0。
 * 返回值：
 *   无。
 */
void ssd1306_v_line(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t height, uint8_t color)
{
    for (uint16_t i = 0; i < height; i++) {
        ssd1306_pixel(dev, x, y + i, color);
    }
}

/* 函数名：ssd1306_rect
 *
 * 函数说明：绘制空心矩形（四条线）。
 * 参数：
 *   dev - 设备句柄。
 *   x   - 左上 X 坐标。
 *   y   - 左上 Y 坐标。
 *   width  - 宽度。
 *   height - 高度。
 *   color  - 非 0 置 1，0 置 0。
 * 返回值：
 *   无。
 */
void ssd1306_rect(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color)
{
    ssd1306_h_line(dev, x, y, width, color);
    ssd1306_h_line(dev, x, y + height - 1, width, color);
    ssd1306_v_line(dev, x, y, height, color);
    ssd1306_v_line(dev, x + width - 1, y, height, color);
}

/* 函数名：ssd1306_fill_rect
 *
 * 函数说明：绘制实心矩形（逐行填充）。
 * 参数：
 *   dev - 设备句柄。
 *   x   - 左上 X 坐标。
 *   y   - 左上 Y 坐标。
 *   width  - 宽度。
 *   height - 高度。
 *   color  - 非 0 置 1，0 置 0。
 * 返回值：
 *   无。
 */
void ssd1306_fill_rect(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color)
{
    for (uint16_t i = 0; i < height; i++) {
        ssd1306_h_line(dev, x, y + i, width, color);
    }
}

/* 函数名：ssd1306_text
 *
 * 函数说明：以 5x8 字体绘制字符串，支持自动换行或截断模式。
 * 参数：
 *   dev - 设备句柄。
 *   str - 要绘制的字符串（ASCII 0x20-0x7F）。
 *   x, y - 起始坐标。
 *   color - 非 0 置 1，0 置 0。
 *   wrap_mode - 0 自动换行，1 截断当前行。
 * 返回值：
 *   无。
 */
void ssd1306_text(ssd1306_t *dev, const char *str, uint16_t x, uint16_t y, uint8_t color, uint8_t wrap_mode)
{
    if (!str) return;
    
    uint16_t cur_x = x;
    uint16_t cur_y = y;
    uint16_t start_x = x;
    
    while (*str) {
        if (*str < 0x20 || *str > 0x7f) {
            str++;
            continue;
        }
        
        /* Check if character will exceed screen width */
        if (cur_x + FONT_CHAR_WIDTH > dev->width) {
            if (wrap_mode == 0) {
                /* Auto wrap: move to next line */
                cur_x = start_x;
                cur_y += FONT_CHAR_HEIGHT;
                
                /* Check if we exceed screen height */
                if (cur_y + FONT_CHAR_HEIGHT > dev->height) {
                    break;  /* Stop drawing if no more vertical space */
                }
            } else {
                /* Truncate mode: stop drawing this line */
                break;
            }
        }
        
        uint8_t char_idx = *str - 0x20;
        const uint8_t *char_data = font_5x8[char_idx];
        
        for (uint8_t i = 0; i < FONT_CHAR_WIDTH; i++) {
            uint8_t col = char_data[i];
            for (uint8_t bit = 0; bit < FONT_CHAR_HEIGHT; bit++) {
                if (col & (1 << bit)) {
                    ssd1306_pixel(dev, cur_x + i, cur_y + bit, color);
                }
            }
        }
        
        cur_x += FONT_TOTAL_WIDTH;
        str++;
    }
}
