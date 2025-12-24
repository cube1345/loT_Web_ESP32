#include "oled_integration.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "oled_integration";

/* I2C configuration - Hardware I2C1 */
#define I2C_NUM           1         /* Hardware I2C port 1 */
#define I2C_SDA_PIN       25        /* GPIO25 - Standard SDA for I2C1 */
#define I2C_SCL_PIN       26        /* GPIO26 - Standard SCL for I2C1 */
#define I2C_FREQ_HZ       700000    /* 700 kHz boosted I2C speed (verify hardware) */
#define OLED_I2C_ADDR     0x3C      /* SSD1306 default I2C address */

/* Refresh rate control */
#define OLED_MIN_REFRESH_MS  100    /* Minimum interval between refreshes (ms) */

oled_context_t g_oled = {0};
static SemaphoreHandle_t oled_mutex = NULL;
static TickType_t last_refresh_tick = 0;

extern const char *FETCH_URL;

/* 函数名：oled_init
 *
 * 函数说明：初始化 I2C 总线与 SSD1306 显示屏，创建互斥并标记初始化状态。
 * 参数：
 *   无。
 * 返回值：
 *   ESP_OK 表示初始化成功，错误时返回对应 esp_err_t。
 */
esp_err_t oled_init(void)
{
    esp_err_t ret;
    
    if (g_oled.initialized) {
        return ESP_OK;
    }
    
    /* Create mutex for thread-safe access */
    oled_mutex = xSemaphoreCreateMutex();
    if (oled_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create OLED mutex");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Initializing I2C master bus");
    
    /* Configure I2C bus */
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    i2c_master_bus_handle_t bus_handle;
    ret = i2c_new_master_bus(&i2c_bus_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "Adding OLED device to I2C bus");
    
    /* Configure SSD1306 device */
    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = OLED_I2C_ADDR,
        .scl_speed_hz = I2C_FREQ_HZ,
    };
    
    i2c_master_dev_handle_t dev_handle;
    ret = i2c_master_bus_add_device(bus_handle, &device_config, &dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SSD1306 device: %s", esp_err_to_name(ret));
        return ret;
    }
    
    /* Initialize SSD1306 display (128x64 pixels, I2C address 0x3C) */
    ret = ssd1306_init(&g_oled.display, dev_handle, 128, 64, OLED_I2C_ADDR, false);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SSD1306: %s", esp_err_to_name(ret));
        return ret;
    }
    
    g_oled.initialized = true;
    ESP_LOGI(TAG, "OLED display initialized successfully");
    
    return ESP_OK;
}

/* 函数名：oled_show_status
 *
 * 函数说明：显示三行状态文本，带刷新频率限制与互斥保护。
 * 参数：
 *   line1 - 第一行文本。
 *   line2 - 第二行文本。
 *   line3 - 第三行文本。
 * 返回值：
 *   无。
 */
void oled_show_status(const char *line1, const char *line2, const char *line3)
{
    if (!g_oled.initialized || oled_mutex == NULL) return;
    
    /* Rate limiting: check if enough time has passed since last refresh */
    TickType_t now = xTaskGetTickCount();
    if ((now - last_refresh_tick) < pdMS_TO_TICKS(OLED_MIN_REFRESH_MS)) {
        return;  /* Skip this refresh to avoid overloading I2C bus */
    }
    
    /* Thread-safe access */
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        ssd1306_clear(&g_oled.display);
        ssd1306_text(&g_oled.display, line1, 0, 0, 1, 1);   /* truncate mode */
        ssd1306_text(&g_oled.display, line2, 0, 16, 1, 1);  /* truncate mode */
        ssd1306_text(&g_oled.display, line3, 0, 32, 1, 1);  /* truncate mode */
        ssd1306_show(&g_oled.display);
        
        last_refresh_tick = xTaskGetTickCount();
        xSemaphoreGive(oled_mutex);
    }
}

/* 函数名：oled_show_connecting
 *
 * 函数说明：显示 Wi-Fi 正在连接的状态信息。
 * 参数：
 *   无。
 * 返回值：
 *   无。
 */
void oled_show_connecting(void)
{
    if (!g_oled.initialized || oled_mutex == NULL) return;
    
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        ssd1306_clear(&g_oled.display);
        ssd1306_text(&g_oled.display, "ESP32 WiFi Demo", 0, 0, 1, 1);  /* truncate mode */
        ssd1306_text(&g_oled.display, "Connecting...", 0, 16, 1, 1);   /* truncate mode */
        ssd1306_show(&g_oled.display);
        last_refresh_tick = xTaskGetTickCount();
        xSemaphoreGive(oled_mutex);
    }
}

/* 函数名：oled_show_connected
 *
 * 函数说明：显示已连接并服务器运行的状态信息。
 * 参数：
 *   无。
 * 返回值：
 *   无。
 */
void oled_show_connected(void)
{
    if (!g_oled.initialized || oled_mutex == NULL) return;
    
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        ssd1306_clear(&g_oled.display);
        ssd1306_text(&g_oled.display, "WiFi Connected!", 0, 0, 1, 1);   /* truncate mode */
        ssd1306_text(&g_oled.display, "Server Running", 0, 16, 1, 1);   /* truncate mode */
        ssd1306_text(&g_oled.display, FETCH_URL, 0, 32, 1, 0);  /* truncate mode */
        ssd1306_show(&g_oled.display);
        last_refresh_tick = xTaskGetTickCount();
        xSemaphoreGive(oled_mutex);
    }
}

/* 函数名：oled_show_connected_with_ip
 *
 * 函数说明：显示已连接状态并附带本机 IP 地址。
 * 参数：
 *   ip_address - 本机 IP 字符串，可为 NULL。
 * 返回值：
 *   无。
 */
void oled_show_connected_with_ip(const char *ip_address)
{
    if (!g_oled.initialized || oled_mutex == NULL) return;
    
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        ssd1306_clear(&g_oled.display);
        ssd1306_text(&g_oled.display, "WiFi Connected!", 0, 0, 1, 1);   /* truncate mode */
        ssd1306_text(&g_oled.display, "Server: Port 443", 0, 12, 1, 1);  /* truncate mode */
        ssd1306_text(&g_oled.display, "IP Address:", 0, 24, 1, 1);       /* truncate mode */
        ssd1306_text(&g_oled.display, ip_address ? ip_address : "N/A", 0, 36, 1, 1);  /* truncate mode */
        ssd1306_show(&g_oled.display);
        last_refresh_tick = xTaskGetTickCount();
        xSemaphoreGive(oled_mutex);
        
        ESP_LOGI(TAG, "OLED displaying IP: %s", ip_address ? ip_address : "N/A");
    }
}

/* 函数名：oled_show_error
 *
 * 函数说明：显示错误标题与错误文本。
 * 参数：
 *   error_text - 错误信息文本。
 * 返回值：
 *   无。
 */
void oled_show_error(const char *error_text)
{
    if (!g_oled.initialized || oled_mutex == NULL) return;
    
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        ssd1306_clear(&g_oled.display);
        ssd1306_text(&g_oled.display, "ERROR", 0, 0, 1, 1);             /* truncate mode */
        ssd1306_text(&g_oled.display, error_text, 0, 16, 1, 0);         /* auto wrap mode */
        ssd1306_show(&g_oled.display);
        last_refresh_tick = xTaskGetTickCount();
        xSemaphoreGive(oled_mutex);
    }
}

/* 函数名：oled_show_joke
 *
 * 函数说明：显示网络获取的笑话文本。
 * 参数：
 *   joke_text - 笑话内容字符串。
 * 返回值：
 *   无。
 */
void oled_show_joke(const char *joke_text)
{
    if (!g_oled.initialized || !joke_text || oled_mutex == NULL) return;
    
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        ssd1306_clear(&g_oled.display);
        ssd1306_text(&g_oled.display, "Joke:", 0, 0, 1, 1);  /* Title: truncate mode */
        
        /* Display joke text with auto wrap (mode 0) */
        ssd1306_text(&g_oled.display, joke_text, 0, 10, 1, 0);
        
        ssd1306_show(&g_oled.display);
        last_refresh_tick = xTaskGetTickCount();
        xSemaphoreGive(oled_mutex);
    }
}

/* 函数名：oled_show_custom_text
 *
 * 函数说明：显示来自 Web 的自定义文本。
 * 参数：
 *   text - 要显示的文本。
 * 返回值：
 *   无。
 */
void oled_show_custom_text(const char *text)
{
    if (!g_oled.initialized || !text || oled_mutex == NULL) return;
    
    if (xSemaphoreTake(oled_mutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        ssd1306_clear(&g_oled.display);
        ssd1306_text(&g_oled.display, "Web Message:", 0, 0, 1, 1);  /* Title */
        
        /* Display custom text with auto wrap */
        ssd1306_text(&g_oled.display, text, 0, 12, 1, 0);
        
        ssd1306_show(&g_oled.display);
        last_refresh_tick = xTaskGetTickCount();
        xSemaphoreGive(oled_mutex);
        
        ESP_LOGI(TAG, "Displayed on OLED: %s", text);
    }
}


