/*
 * @Descripttion: LCD屏幕驱动和基本显示
 * @version: 
 * @Author: Newt
 * @Date: 2022-09-27 14:52:20
 * @LastEditors: Newt
 * @LastEditTime: 2022-10-17 16:18:34
 */

#ifndef __ST75256__H__
#define __ST75256__H__

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define RS  5
#define RST 4

int lf_lcd_init();
void lf_lcd_clear_screen();
void lf_lcd_clear_data(int x, int y, int xEnd, int yEnd);
void lf_lcd_8x16_charcters(int x, int y, uint8_t num);
void lf_lcd_8x16_numbers(int x, int y, uint8_t num);
void lf_lcd_16x16_charcters(int x, int y, uint8_t num);
void lf_lcd_display_picture(int x, int y);
/* for Tetris only */
void lf_tetris_draw_block(int x, int y);
void lf_tetris_clear_block(int x, int y);
void lf_tetris_reset_overline();
void lf_tetris_frame_line();

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* __ST75256__H__ */