#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SSD1306 Display Commands */
#define SET_CONTRAST        0x81
#define SET_ENTIRE_ON       0xa4
#define SET_NORM_INV        0xa6
#define SET_DISP            0xae
#define SET_MEM_ADDR        0x20
#define SET_COL_ADDR        0x21
#define SET_PAGE_ADDR       0x22
#define SET_DISP_START_LINE 0x40
#define SET_SEG_REMAP       0xa0
#define SET_MUX_RATIO       0xa8
#define SET_COM_OUT_DIR     0xc0
#define SET_DISP_OFFSET     0xd3
#define SET_COM_PIN_CFG     0xda
#define SET_DISP_CLK_DIV    0xd5
#define SET_PRECHARGE       0xd9
#define SET_VCOM_DESEL      0xdb
#define SET_CHARGE_PUMP     0x8d

/* Font configuration - can be customized */
#define FONT_CHAR_WIDTH     5       /* Character bitmap width (pixels) */
#define FONT_CHAR_HEIGHT    8       /* Character bitmap height (pixels) */
#define FONT_CHAR_SPACING   1       /* Space between characters (pixels) */
#define FONT_TOTAL_WIDTH    (FONT_CHAR_WIDTH + FONT_CHAR_SPACING)  /* Total width per char *//* I2C Control Bytes */
#define I2C_CMD_BYTE        0x80  /* Co=1, D/C#=0 */
#define I2C_DATA_BYTE       0x40  /* Co=0, D/C#=1 */

typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t pages;
    uint8_t *buffer;
    i2c_master_dev_handle_t i2c_dev;
    uint8_t i2c_addr;
    bool external_vcc;
    /* Dirty tracking for incremental refresh */
    uint8_t dirty_flags;              /* bit0: any dirty, other bits reserved */
    uint8_t page_dirty[8];            /* per-page dirty flag (max 8 pages for 64px height) */
    uint8_t dirty_col_start[8];       /* per-page first dirty column */
    uint8_t dirty_col_end[8];         /* per-page last dirty column */
} ssd1306_t;

/* Initialization and Control */
esp_err_t ssd1306_init(ssd1306_t *dev, i2c_master_dev_handle_t i2c_dev,
                       uint16_t width, uint16_t height, uint8_t i2c_addr,
                       bool external_vcc);

void ssd1306_deinit(ssd1306_t *dev);

esp_err_t ssd1306_power_on(ssd1306_t *dev);
esp_err_t ssd1306_power_off(ssd1306_t *dev);

esp_err_t ssd1306_set_contrast(ssd1306_t *dev, uint8_t contrast);
esp_err_t ssd1306_invert(ssd1306_t *dev, bool invert);

/* Display Update */
esp_err_t ssd1306_show(ssd1306_t *dev);

/* Framebuffer Operations */
void ssd1306_fill(ssd1306_t *dev, uint8_t color);
void ssd1306_clear(ssd1306_t *dev);

void ssd1306_pixel(ssd1306_t *dev, uint16_t x, uint16_t y, uint8_t color);
void ssd1306_h_line(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t width, uint8_t color);
void ssd1306_v_line(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t height, uint8_t color);

void ssd1306_rect(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);
void ssd1306_fill_rect(ssd1306_t *dev, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t color);

/* Text Rendering (5x8 font) */
void ssd1306_text(ssd1306_t *dev, const char *str, uint16_t x, uint16_t y, uint8_t color, uint8_t wrap_mode);
/* wrap_mode: 0 = auto wrap to next line, 1 = truncate (no wrap) */
#endif

