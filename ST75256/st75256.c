/*
 * @Descripttion:LCD屏幕驱动和基本显示
 * @version:
 * @Author: Newt
 * @Date: 2022-09-27 14:51:12
 * @LastEditors: Newt
 * @LastEditTime: 2022-12-08 17:36:43
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <lf_timer.h>
#include <lf_gpio.h>

#include "st75256.h"
#include "simulate_spi.h"
#include "dis_lib.h"

#define ENABLE_RS    lf_gpio_enable_output(RS, 0, 0)
#define ENABLE_RST   lf_gpio_enable_output(RST, 0, 0)
#define SET_RS_LOW   lf_gpio_output_set(RS, 0)
#define SET_RS_HIGH  lf_gpio_output_set(RS, 1)
#define SET_RST_LOW  lf_gpio_output_set(RST, 0)
#define SET_RST_HIGH lf_gpio_output_set(RST, 1)

static void lf_lcd_send_cmd(uint8_t data)
{
    SET_CS_LOW;
    SET_RS_LOW;
    simulate_spi_transmit(&data, 1);
    SET_CS_HIGH;
}

static void lf_lcd_send_data(uint8_t data)
{
    SET_CS_LOW;
    SET_RS_HIGH;
    simulate_spi_transmit(&data, 1);
    SET_CS_HIGH;
}

static int lcd_gpio_init()
{
    simulate_spi_init();
    ENABLE_RS;
    ENABLE_RST;
    return 0;
}


/**
 * @brief: lcd initiation
 * @return {*} 0 means success
 */
int lf_lcd_init()
{
    if (lcd_gpio_init() != 0) {
        printf("lcd gpio init failed\r\n");
        return 1;
    }

    SET_RST_LOW;
    vTaskDelay(100);
    SET_RST_HIGH;
    vTaskDelay(100);

    lf_lcd_send_cmd(0x30);  // EXT=0
    lf_lcd_send_cmd(0x94);  // Sleep out
    lf_lcd_send_cmd(0x31);  // EXT=1
    lf_lcd_send_cmd(0xD7);  // Autoread disable
    lf_lcd_send_data(0X9F); //
    lf_lcd_send_cmd(0x32);  // Analog SET
    lf_lcd_send_data(0x00); // OSC Frequency adjustment
    lf_lcd_send_data(0x01); // Frequency on booster capacitors->6KHz
    lf_lcd_send_data(0x03); // Bias=1/11
    lf_lcd_send_cmd(0x20);  // Gray Level

    lf_lcd_send_cmd(0x31);  // Analog SET
    lf_lcd_send_cmd(0xf2);  //温度补偿
    lf_lcd_send_data(0x1e); // OSC Frequency adjustment
    lf_lcd_send_data(0x28); // Frequency on booster capacitors->6KHz
    lf_lcd_send_data(0x32); //

    lf_lcd_send_data(0x01);
    lf_lcd_send_data(0x03);
    lf_lcd_send_data(0x05);
    lf_lcd_send_data(0x07);
    lf_lcd_send_data(0x09);
    lf_lcd_send_data(0x0b);
    lf_lcd_send_data(0x0d);
    lf_lcd_send_data(0x10);
    lf_lcd_send_data(0x11);
    lf_lcd_send_data(0x13);
    lf_lcd_send_data(0x15);
    lf_lcd_send_data(0x17);
    lf_lcd_send_data(0x19);
    lf_lcd_send_data(0x1b);
    lf_lcd_send_data(0x1d);
    lf_lcd_send_data(0x1f);
    lf_lcd_send_cmd(0x30);  // EXT=0
    lf_lcd_send_cmd(0x75);  // Page Address setting
    lf_lcd_send_data(0X00); // XS=0
    lf_lcd_send_data(0X14); // XE=159 0x28
    lf_lcd_send_cmd(0x15);  // Clumn Address setting
    lf_lcd_send_data(0X00); // XS=0
    lf_lcd_send_data(0Xff); // XE=256
    lf_lcd_send_cmd(0xBC);  // Data scan direction
    lf_lcd_send_data(0x00); // MX.MY=Normal
    lf_lcd_send_data(0xA6);
    lf_lcd_send_cmd(0xCA);  // Display Control
    lf_lcd_send_data(0X00); //
    lf_lcd_send_data(0X9F); // Duty=160
    lf_lcd_send_data(0X20); // Nline=off
    lf_lcd_send_cmd(0xF0);  // Display Mode
    lf_lcd_send_data(0X10); // 10=Monochrome Mode,11=4Gray
    lf_lcd_send_cmd(0x81);  // EV control
    lf_lcd_send_data(0x0a); // VPR[5-0] //可设置范围 0x00~0x3f,每格电压是 0.04V
    lf_lcd_send_data(0x04); // VPR[8-6] //可设置范围 0x00~0x07
    lf_lcd_send_cmd(0x20);  // Power control
    lf_lcd_send_data(0x0B); // D0=regulator ; D1=follower ; D3=booste, on:1 off:0

    lf_timer_delay_us(100);
    lf_lcd_send_cmd(0xAF); // Display on

    return 0;
}

/**
 * @brief: address ordinate
 * @param {int} x   
 * @param {int} y
 * @param {int} xEnd
 * @param {int} yEnd
 */
static void lcd_select_address(int x, int y, int xEnd, int yEnd)
{
    // x = x - 1;
    y += 8;     /* y初始值为8 */
    yEnd += 8;
    lf_lcd_send_cmd(0x15);
    lf_lcd_send_data(x);
    lf_lcd_send_data(xEnd);

    lf_lcd_send_cmd(0x75);
    lf_lcd_send_data(y);
    lf_lcd_send_data(yEnd);

    lf_lcd_send_cmd(0x30);
    lf_lcd_send_cmd(0x5c);
}

void test_scan_direction()
{
    lcd_select_address(10, 0, 10, 11);
    for (uint8_t i = 0; i < 96; i++) {
        lf_lcd_send_data(0xff);
    }
}

/**
 * @brief: clear screen
 */
void lf_lcd_clear_screen()
{
    int i, j;
    lcd_select_address(0, 0, 255, 11);
    for (i = 0; i < 12; i++) {
        for (j = 0; j < 256; j++) {
            lf_lcd_send_data(0x00);
        }
    }
}

void lf_lcd_clear_data(int x, int y, int xEnd, int yEnd)
{
    lcd_select_address(x, y, xEnd, yEnd);
    for (int i = x; i <= xEnd; i++) {
        for (int j = y; j <= yEnd; j++) {
            lf_lcd_send_data(0x00);
        }
    }
}

/**
 * @brief: display English characters
 * @param {int} x
 * @param {int} y
 * @param {uint8_t} num numbers of characters
 */
void lf_lcd_8x16_charcters(int x, int y, uint8_t num)
{
        lcd_select_address(x, y, x + 16, y + 1);
        for (uint8_t j = 0; j < 16; j++) {
            lf_lcd_send_data(character_lib[num][j]);
        }
}

/**
 * @brief: display numbers
 * @param {int} x
 * @param {int} y
 * @param {uint8_t} num
 */
void lf_lcd_8x16_numbers(int x, int y, uint8_t num)
{
        lcd_select_address(x, y, x + 16, y + 1);
        for (uint8_t j = 0; j < 16; j++) {
            lf_lcd_send_data(num_lib[num][j]);
        }
        y += 1;
}

uint8_t character_CN[] = {
    0x00,0x00,0x00,0xFE,0xA9,0xAA,0xA8,0xA8,0xA8,0xFF,0x00,0x08,0x04,0xFE,0x00,0x00,
		0x00,0x02,0x22,0x2A,0x2A,0x2A,0xFE,0x2A,0x2A,0x2A,0x22,0x00,0x00,0x33,0x42,0x02,
	/* 0xC7EC [��]   [3648]*/
		0x00,0x01,0x01,0x02,0x04,0x18,0x60,0x80,0x60,0x18,0x04,0x02,0x01,0xF0,0x0C,0x02,
		0x00,0x20,0x21,0x21,0x21,0x21,0x21,0x6F,0xA1,0x21,0x21,0x21,0x20,0x3F,0x00,0x00,
	/* 0xC7ED [��]   [3649]*/
		0x00,0x0C,0x90,0xA0,0x80,0xFE,0x81,0x82,0xB0,0x08,0x04,0x10,0x10,0xF8,0x0C,0x08,
		0x00,0x20,0x27,0x24,0x24,0x64,0xA4,0x24,0x27,0x20,0x00,0x22,0x22,0x3F,0x22,0x22,
	/* 0xC7EE [��]   [3650]*/
		0x00,0x00,0x00,0x00,0xFC,0x82,0x81,0x82,0x80,0xC0,0xB0,0x88,0x84,0x82,0x81,0x00,
		0x00,0x30,0x28,0x22,0x24,0x28,0x20,0x20,0x60,0xA3,0x20,0x28,0x24,0x22,0x30,0x08,
	/* 0xC7EF [��]   [3651]*/
		0x00,0x01,0x02,0x04,0x18,0x60,0x80,0x70,0x0C,0x02,0x61,0x80,0xFF,0x80,0x60,0x10,
		0x00,0x00,0x0C,0x02,0x01,0x00,0xFF,0x00,0x0E,0x01,0x44,0xC4,0x7F,0x25,0x24,0x24,
	/* 0xC7F0 [��]   [3652]*/
		0x00,0x02,0x02,0x02,0x02,0x02,0xFE,0x02,0x02,0x02,0x02,0x02,0xFE,0x02,0x02,0x02,
		0x00,0x00,0x01,0x41,0xC1,0x41,0x41,0x21,0x21,0x21,0x21,0x21,0x3F,0x00,0x00,0x00,
	/* 0xC7F1 [��]   [3653]*/
		0x00,0xE0,0x10,0x08,0x10,0xFF,0x00,0x08,0x08,0xF8,0x08,0x04,0x04,0xFC,0x06,0x04,
		0x00,0x60,0x5B,0x44,0x40,0x7F,0x00,0x02,0x42,0x43,0x22,0x22,0x22,0x3F,0x00,0x00,
	/* 0xC7F2 [��]   [3654]*/
		0x00,0x08,0x90,0x60,0x80,0xFE,0x41,0xA2,0x10,0x08,0x08,0x08,0xF8,0x04,0x06,0x04,
		0x00,0x09,0x68,0x88,0x09,0xFF,0x08,0x09,0x0A,0x08,0x21,0x21,0x3F,0x21,0x21,0x20,
	/* 0xC7F3 [��]   [3655]*/
		0x00,0x08,0x08,0x10,0x20,0x40,0x80,0x00,0xFE,0x81,0x42,0x20,0x10,0x08,0x08,0x00,
		0x00,0x00,0x10,0x14,0x52,0x91,0x10,0x11,0xFF,0x10,0x10,0x11,0x12,0x14,0x10,0x00,
	/* 0xC7F4 [��]   [3656]*/
		0x00,0x00,0xFF,0x02,0x02,0x3A,0xC2,0x02,0x02,0x82,0x62,0x12,0x0A,0x02,0xFF,0x00,
		0x00,0x00,0x7F,0x40,0x40,0x40,0x40,0x41,0x5E,0x41,0x40,0x40,0x40,0x40,0x7F,0x00,
	/* 0xC7F5 [��]   [3657]*/
		0x00,0x00,0xFF,0x52,0x52,0x52,0x92,0x12,0x12,0x12,0x12,0x92,0x52,0x32,0xFF,0x00,
		0x00,0x10,0x13,0x12,0xD2,0x32,0x1F,0x12,0x12,0x12,0x1F,0x72,0x92,0x12,0x13,0x10,
	/* 0xC7F6 [��]   [3658]*/
		0x00,0x00,0xFF,0x02,0x32,0x42,0x82,0x62,0x12,0x0A,0xFF,0x00,0x80,0x7E,0x20,0x20,
		0x00,0x00,0x7F,0x40,0x40,0x40,0x5F,0x40,0x40,0x40,0x7F,0x00,0x31,0x40,0x06,0x08,
	/* 0xC7F7 [��]   [3659]*/
		0x00,0x02,0xF2,0x92,0x92,0x92,0x92,0x92,0x12,0x22,0x22,0xFE,0x04,0xF8,0x06,0x01,
		0x00,0x00,0x07,0x34,0x2C,0x24,0xE4,0x14,0x08,0x12,0x12,0xFF,0x12,0x12,0x12,0x02,
	/* 0xC7F8 [��]   [3660]*/
		0x00,0x02,0x02,0x02,0x32,0x42,0x82,0x02,0x82,0x42,0x22,0x12,0x02,0x02,0xFE,0x00,
		0x00,0x00,0x40,0x40,0x58,0x44,0x42,0x41,0x42,0x44,0x48,0x40,0x40,0x40,0x7F,0x00,
};
/* 16x16 */
static void character16x16_rotate_90(uint8_t *num_lib, uint8_t *w_data)
{
    uint8_t buf[32] = {0};
    uint8_t num_buf[32] = {0};
    uint8_t value = 0x80;
    uint8_t y = 8;

    memcpy(num_buf, num_lib + 16, 16);
    memcpy(num_buf + 16, num_lib, 16);

    for (uint8_t i = 0; i < 32; i++) {
        if (i == 0 || i == 8 || i == 16 || i == 24) {
            value = 0x80;
            if (i == 0) {
                y = 8;
            }
            else if ( i == 8) {
                y = 24;
            }
            else if (i == 16) {
                y = 0;
            } 
            else if (i == 24) {
                y = 16;
            }
        }
        else {
            value >>= 1;
        }
        for (uint8_t j = 0; j < 8; j++) {
            if (num_buf[y + j] & value) {
                buf[i] |= 0x80 >> j;
            }
        }
    }

    for (uint8_t k = 0; k < 16; k++) {
        w_data[k] = buf[16 + k];
        w_data[16 + k] = buf[k];
    }
}

void test_rotate_90(int x, int y, int num)
{
    uint8_t w_data[32] = {0};
    character16x16_rotate_90(character_CN + 32 * num, w_data);
    // memcpy(w_data, character_CN, 32);
    lcd_select_address(x, y, x + 15, y + 1);
    for (uint8_t j = 0; j < 32; j++) {
        lf_lcd_send_data(w_data[j]);
    }
}

void lf_lcd_16x16_charcters(int x, int y, uint8_t num)
{
        lcd_select_address(x, y, x + 15, y + 1);
        for (uint8_t j = 0; j < 32; j++) {
            lf_lcd_send_data(logo_lib[num][j]);
        }
}

void lf_lcd_display_picture(int x, int y)
{
    uint8_t *p = gImage_lskj;
    lcd_select_address(x, y, 189, 6);
    for (int i = 0; i < 6; i++) {
        for (int j = 3; j < 189; j++) {
            lf_lcd_send_data(*p);
            p++;
        }
    }
}


void lf_tetris_draw_block(int x, int y)
{
    /* 0<= x <=19; 0<= y <= 9 */
    int x1 = 95 + 8 * x ;
    int x2 = 95 + 8 * x + 7;
    int y1 = 10 - y ;
    lcd_select_address(x1, y1, x2, y1);
    for (uint8_t i = 0; i < 8; i++) {
        if (i == 0 || i == 7) {
            lf_lcd_send_data(0xff);
        }
        else if (i == 1 || i == 6) {
            lf_lcd_send_data(0x81);
        }
        else if (i > 1 && i < 6) {
            lf_lcd_send_data(0xbd);
        }
    }
}

void lf_tetris_clear_block(int x, int y)
{
    /* 0<= x <=19; 0<= y <= 9 */
    int x1 = 95 + 8 * x;
    int x2 = 95 + 8 * x + 7;
    int y1 = 10 - y;
    lcd_select_address(x1, y1, x2, y1);
    for (uint8_t i = 0; i < 8; i++) {
        lf_lcd_send_data(0x00);
    }
}

void lf_tetris_reset_overline()
{
    int i, j;
    lcd_select_address(85, 0, 94, 11);
    for (i = 0; i < 12; i++) {
        for (j = 0; j < 10; j++) {
            lf_lcd_send_data(0x00);
        }
    }

    lcd_select_address(94, 0, 94, 11);
    for (i = 0; i < 12; i++) {
        lf_lcd_send_data(0xff);
    }
}

void lf_tetris_frame_line()
{
    lcd_select_address(94, 11, 255, 11);
    for (uint8_t i = 0; i < 165; i++) {
        lf_lcd_send_data(0x01);
    }

    lcd_select_address(94, 0, 255, 0);
    for (uint8_t i = 0; i < 165; i++) {
        lf_lcd_send_data(0x80);
    }

    lcd_select_address(255, 0, 255, 11);
    for (uint8_t i = 0; i < 12; i++) {
        lf_lcd_send_data(0xff);
    }

    lcd_select_address(94, 0, 94, 11);
    for (uint8_t i = 0; i < 12; i++) {
        lf_lcd_send_data(0xff);
    }
}