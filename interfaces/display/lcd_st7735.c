#include "nrf_error.h"
#include "nrf_gfx.h"
#include "nrf_lcd.h"

#include <stdlib.h>
#include <string.h>

#define GRAY 0xC618

static const char *test_text = "Moisture (%): ";
static unsigned char buf[2];

extern const nrf_gfx_font_desc_t orkney_8ptFontInfo;
extern const nrf_lcd_t nrf_lcd_st7735;

static const nrf_gfx_font_desc_t *p_font = &orkney_8ptFontInfo;
static const nrf_lcd_t *p_lcd = &nrf_lcd_st7735;

void gfx_init(void) {
  APP_ERROR_CHECK(nrf_gfx_init(p_lcd));
}

void background_set(void) {
  nrf_gfx_rotation_set(p_lcd, NRF_LCD_ROTATE_270);
  nrf_gfx_invert(p_lcd, true);
}

void moisture_print(int percentage) {
  itoa(percentage, buf, 10); // Convert to string

  nrf_gfx_point_t label = NRF_GFX_POINT(5, nrf_gfx_height_get(p_lcd) - 100);
  APP_ERROR_CHECK(nrf_gfx_print(p_lcd, &label, 0, test_text, p_font, true));

  nrf_gfx_point_t value_start = NRF_GFX_POINT(100, nrf_gfx_height_get(p_lcd) - 100);
  APP_ERROR_CHECK(nrf_gfx_print(p_lcd, &value_start, 0, buf, p_font, true));
}

void screen_clear(void) {
  nrf_gfx_screen_fill(p_lcd, GRAY);
}