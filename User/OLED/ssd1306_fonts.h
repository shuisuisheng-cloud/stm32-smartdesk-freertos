#ifndef __SSD1306_FONTS_H__
#define __SSD1306_FONTS_H__

#include <stdint.h>
#include "ssd1306_conf.h"
#include <stddef.h>

/* 字体结构体定义 */
typedef struct {
    const uint8_t width;
    const uint8_t height;
    const uint16_t *data;
    const uint8_t *char_width;
} SSD1306_Font_t;

/* Include only needed fonts */
#ifdef SSD1306_INCLUDE_FONT_6x8
extern const SSD1306_Font_t Font_6x8;
#endif

#ifdef SSD1306_INCLUDE_FONT_7x10
extern const SSD1306_Font_t Font_7x10;
#endif

#ifdef SSD1306_INCLUDE_FONT_11x18
extern const SSD1306_Font_t Font_11x18;
#endif

#ifdef SSD1306_INCLUDE_FONT_16x26
extern const SSD1306_Font_t Font_16x26;
#endif

#ifdef SSD1306_INCLUDE_FONT_16x24
extern const SSD1306_Font_t Font_16x24;
#endif

#ifdef SSD1306_INCLUDE_FONT_16x15
extern const SSD1306_Font_t Font_16x15;
#endif

#endif