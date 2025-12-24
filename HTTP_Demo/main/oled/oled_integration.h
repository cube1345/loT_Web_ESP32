/*
 * 集成 SSD1306 OLED 显示到 HTTP Demo
 * 
 * 使用 I2C0:
 *   SDA = GPIO 21
 *   SCL = GPIO 22
 * 
 * SSD1306 OLED:
 *   VCC = 3.3V
 *   GND = GND
 *   SDA = GPIO 21
 *   SCL = GPIO 22
 */

#ifndef OLED_INTEGRATION_H
#define OLED_INTEGRATION_H

#include "ssd1306.h"
#include "esp_log.h"

typedef struct {
    ssd1306_t display;
    bool initialized;
} oled_context_t;

/* Global OLED context */
extern oled_context_t g_oled;

/* Initialize I2C and OLED display */
esp_err_t oled_init(void);

/* Display messages on OLED */
void oled_show_status(const char *line1, const char *line2, const char *line3);
void oled_show_joke(const char *joke_text);
void oled_show_connecting(void);
void oled_show_connected(void);
void oled_show_connected_with_ip(const char *ip_address);  /* 显示IP地址 */
void oled_show_error(const char *error_text);
void oled_show_custom_text(const char *text);  /* 显示自定义文本 */

#endif /* OLED_INTEGRATION_H */
