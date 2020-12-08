#ifndef LCD_ST7735_H
#define LCD_ST7735_H

void gfx_init(void);
void background_set(void);
void moisture_print(int16_t adc);
void screen_clear(void);

#endif