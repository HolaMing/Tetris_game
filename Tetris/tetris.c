/*
 * @Descripttion:俄罗斯方块算法
 * @version:
 * @Author: Newt
 * @Date: 2022-09-28 18:00:07
 * @LastEditors: Newt
 * @LastEditTime: 2022-10-28 13:56:29
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <lf_timer.h>
#include <lf_sec.h>
#include <blog.h>

#include "tetris.h"
#include "st75256.h"

/* playfield size */
#define ROW 20
#define COL 10

static int g_next_shape; /* next mino's shape */
static int g_next_form;  /* next mino's form */


/* record each blocks' status in palayfield
 * x start from [1]
 * x end in [ROW]
 * y start from [1]
 * y end in [COL]
 * [0] and [ROW + 1] are the boundaries
 */
uint8_t g_data[ROW + 2][COL + 2];

/* mino info */
struct {
    uint8_t coordinate[4][4];
} mino_t[7][4];

/**
 * @brief: init palayfield
 */
static void playfiled_data_init()
{
    for (int i = ROW + 1; i >= 0; i--) {
        for (int j = COL + 1; j >= 0; j--) {
            g_data[i][j] = 0;
        }
    }
    for (int i = COL + 1; i >= 0; i--) {
        g_data[ROW + 1][i] = 1;
    }
    for (int i = ROW + 1; i >= 1; i--) {
        g_data[i][0] = 1;
        g_data[i][COL + 1] = 1;
    }
}

/**
 * @brief: Mino info configuration
 */
static void tetromino_info_init()
{
    //“T”形
    for (int i = 0; i <= 2; i++)
        mino_t[0][0].coordinate[1][i] = 1;
    mino_t[0][0].coordinate[2][1] = 1;

    //“L”形
    for (int i = 1; i <= 3; i++)
        mino_t[1][0].coordinate[i][1] = 1;
    mino_t[1][0].coordinate[3][2] = 1;

    //“J”形
    for (int i = 1; i <= 3; i++)
        mino_t[2][0].coordinate[i][2] = 1;
    mino_t[2][0].coordinate[3][1] = 1;

    for (int i = 0; i <= 1; i++) {
        //“Z”形
        mino_t[3][0].coordinate[1][i] = 1;
        mino_t[3][0].coordinate[2][i + 1] = 1;
        //“S”形
        mino_t[4][0].coordinate[1][i + 1] = 1;
        mino_t[4][0].coordinate[2][i] = 1;
        //“O”形
        mino_t[5][0].coordinate[1][i + 1] = 1;
        mino_t[5][0].coordinate[2][i + 1] = 1;
    }

    //“I”形
    for (int i = 0; i <= 3; i++)
        mino_t[6][0].coordinate[i][1] = 1;

    int temp[4][4];
    for (int shape = 0; shape < 7; shape++) // 7种形状
    {
        for (int form = 0; form < 3; form++) // 4种形态（已经有了一种，这里每个还需增加3种）
        {
            //获取第form种形态
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    temp[i][j] = mino_t[shape][form].coordinate[i][j];
                }
            }
            //将第form种形态顺时针旋转，得到第form+1种形态
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    mino_t[shape][form + 1].coordinate[i][j] = temp[3 - j][i];
                }
            }
        }
    }
}

/**
 * @brief: game initiation
 */
void lf_tetris_init_game()
{
    int y1, y2, y3;
    y1 = 8;
    y2 = 8;
    y3 = 7;
    /* clear screen */
    lf_lcd_clear_screen();
    /* frame lines */
    lf_tetris_frame_line();
    lf_tetris_reset_overline();
    /* characters displayed */
    /* 分数 */
    for (uint8_t i = 21; i < 23; i++) {
        lf_lcd_16x16_charcters(0, y1, i);
        y1 += 2;
    }
    /* ： */
    lf_lcd_8x16_charcters(0, 7, 0);
    /* 对手 */
    for (uint8_t i = 23; i < 25; i++) {
        lf_lcd_16x16_charcters(32, y2, i);
        y2 += 2;
    }
    /* ： */
    lf_lcd_8x16_charcters(32, 7, 0);
    /* my score: */
    // for (uint8_t i = 0; i < 9; i++) {
    //     lf_lcd_8x16_charcters(0, y1, i);
    //     y1++;
    // }
    /* rival: */
    // for (uint8_t i = 9; i < 15; i++) {
    //     lf_lcd_8x16_charcters(32, y2, i);
    //     y2++;
    // }
    /* NEXT: */
    for (uint8_t i = 15; i < 20; i++) {
        lf_lcd_8x16_charcters(64, y3, i);
        y3++;
    }
    /* data initiation */
    g_myscore = 0;
    memset(g_data, 0, sizeof(g_data));
    memset(mino_t, 0, sizeof(mino_t));
    playfiled_data_init();
    tetromino_info_init();
    blog_info("init game\r\n");
}

void lf_mino_draw(int shape, int form, int x, int y)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (mino_t[shape][form].coordinate[i][j] == 1) {
                lf_tetris_draw_block(x + i, y + j);
            }
        }
    }
}

void lf_mino_clear(int shape, int form, int x, int y)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (mino_t[shape][form].coordinate[i][j] == 1) {
                lf_tetris_clear_block(x + i, y + j);
            }
        }
    }
}

/**
 * @brief: preview next mino in the location
 * @param {int} x = -8
 * @param {int} y = 5
 * @return {*}
 */
void lf_mino_preview_next(int x, int y)
{
    // srand(lf_timer_now_us());
    // g_next_shape = rand() % 7;
    // g_next_form = rand() % 4;
    g_next_shape = lf_rand() % 7;
    g_next_form = lf_rand() % 4;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (mino_t[g_next_shape][g_next_form].coordinate[i][j] == 1) {
                lf_tetris_draw_block(x + i, y + j);
            }
        }
    }
}

/**
 * @brief: clear previewed mino in the location
 * @param {int} x = -8
 * @param {int} y = 5
 * @return {*}
 */
void lf_mino_clear_next(int x, int y)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            lf_tetris_clear_block(x + i, y + j);
        }
    }
}

/**
 * @brief: create new mino in the location
 * @param {int} x = -1
 * @param {int} y = 3
 * @return {*}
 */
void lf_mino_create_new(int x, int y)
{
    g_shape = g_next_shape;
    g_form = g_next_form;
    lf_mino_draw(g_shape, g_form, x, y);
    vTaskDelay(100);
}

/**
 * @brief: judge mino's location is legal or not
 * @param {int} shape
 * @param {int} form
 * @param {int} x
 * @param {int} y
 * @return {*} 0 means legal, and vice versa
 */
int lf_mino_judge_legal(int shape, int form, int x, int y)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if ((mino_t[shape][form].coordinate[i][j] == 1) && (g_data[1 + x + i][1 + y + j] == 1))
                return 1;
        }
    }
    return 0;
}

/**
 * @brief: record the  fixed mino's location
 * @param {int} x
 * @param {int} y
 * @return {*}
 */
void lf_mino_record_location(int x, int y)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (mino_t[g_shape][g_form].coordinate[i][j] == 1) {
                g_data[x + 1 + i][y + 1 + j] = 1;
            }
        }
    }
}

/**
 * @brief: key control
 * @param {key_e} mode
 * @return {*}
 */
int lf_mino_move_control(key_e mode)
{
    switch (mode) {
        case RIGHT:
            if (lf_mino_judge_legal(g_shape, g_form, g_x, g_y + 1) == 0) {
                lf_mino_clear(g_shape, g_form, g_x, g_y);
                g_y++;
                return 0;
            }
            break;
        case LEFT:
            if (lf_mino_judge_legal(g_shape, g_form, g_x, g_y - 1) == 0) {
                lf_mino_clear(g_shape, g_form, g_x, g_y);
                g_y--;
                return 0;
            }
            break;
        case DOWN:
            if (lf_mino_judge_legal(g_shape, g_form, g_x + 1, g_y) == 0) {
                lf_mino_clear(g_shape, g_form, g_x, g_y);
                g_x++;
                return 0;
            }
            break;
        case SPIN:
            if (lf_mino_judge_legal(g_shape, (g_form + 1) % 4, g_x + 1, g_y) == 0) {
                lf_mino_clear(g_shape, g_form, g_x, g_y);
                g_form = (g_form + 1) % 4;
                return 0;
            }
            break;
        default:
            break;
    }
    return 1;
}

/**
 * @brief: judge if get score or not
 * @return {*}
 */
int lf_tetris_judge_score()
{
    for (int i = ROW; i > 0; i--) {
        int sum = 0; /* the num of i-th row's blocks */
        for (int j = 1; j < COL + 1; j++) {
            sum += g_data[i][j];
        }
        if (sum == 0) {
            break;
        }
        if (sum == COL) { 
            g_myscore += 10;
            for (int j = 1; j < COL + 1; j++) {
                g_data[i][j] = 0;
            }
            for (int m = i; m > 0 ; m--) {
                sum = 0; /* the last row's blocks */
                for (int n = 1; n < COL + 1; n++) {
                    sum += g_data[m - 1][n];
                    g_data[m][n] = g_data[m-1][n];
                    if (g_data[m][n] == 1) {
                        lf_tetris_draw_block(m - 1, n - 1);
                    }
                    else {
                        lf_tetris_clear_block(m - 1, n - 1);
                    }
                }
                if (sum == 0) {
                    return 1; /* coz the m-th row has cleared, need to judge the new m-th row again */
                }
            } 
        }
    }
    printf("score is %d\r\n", g_myscore);
    return 0;
}

/**
 * @brief: check game over or not
 * @return {*} 0 means failed, and vice versa
 */
int lf_tetris_check_failed()
{
    for (int i = 1; i < COL + 1; i++) {
        if (g_data[0][i] == 1) {
            lf_tetris_reset_overline(); /* reset the gamefield's overline, declared in another file */
            vTaskDelay(500);
            for (int j = 0; j <= 19; j++) {
                for (int k = 0; k <= 9; k++) {
                    vTaskDelay(20 - j);
                    lf_tetris_draw_block(j, k);
                }
            }
            vTaskDelay(500);
            for (int j = 19; j >= 0; j--) {
                for (int k = 9; k >= 0; k--) {
                    vTaskDelay(j);
                    lf_tetris_clear_block(j, k);
                }
            }
            return 0;
        }
    }
    return 1;
}

/**
 * @brief: welcome interface
 */
void lf_tetris_welcome()
{
    int y = 1;
    for (int i = 0; i < 5; i++) {
        lf_lcd_16x16_charcters(90, y, i); /* 俄罗斯方块 */
        y += 2;
    }
    y = 0;
    for (int i = 5; i < 11; i++) {
        lf_lcd_16x16_charcters(40, y, i); /* TETRIS */
        y += 2;
    }
    y = 2;
    for (int i = 20; i < 27 ; i++) {
        lf_lcd_8x16_charcters(180, y, i); /* WELCOME */
        y++;
    }
    tetromino_info_init();
    while (1) {
            lf_mino_preview_next(3, 1);
            lf_mino_preview_next(3, 6);
            vTaskDelay(1000);
            lf_mino_clear_next(3, 1); 
            lf_mino_clear_next(3, 6); 
    }
    vTaskDelete(NULL);
}

void lf_tetris_start()
{
    int y = 1;
    lf_lcd_clear_screen();
    for (int i = 11; i < 16; i++) {
        lf_lcd_16x16_charcters(90, y, i);
        y += 2;
    }
    y = 1;
    for (int i = 16; i < 21; i++) {
        lf_lcd_16x16_charcters(130, y, i);
        y += 2;
    }
    vTaskDelay(1000);
    lf_lcd_clear_screen();
}

void lf_tetris_over()
{
    int y = 2;
    /* game over */
    lf_lcd_clear_screen();
    for (int i = 25; i < 29; i++) {
        lf_lcd_16x16_charcters(90, y, i);
        y += 2;
    }
    y = 2;
    for (int i = 29; i < 33; i++) {
        lf_lcd_16x16_charcters(110, y, i);
        y += 2;
    }
    /* press YELLOW button then restart */
    y = 0;
    for (int i = 27; i < 39; i++) {
        lf_lcd_8x16_charcters(150, y, i);
        y++;
    }
    y = 2;
    for (int i = 40; i < 46; i++) {
        lf_lcd_8x16_charcters(166, y, i);
        y++;
    }
    y = 0;
    for (int i = 46; i < 58; i++) {
        lf_lcd_8x16_charcters(182, y, i);
        y++;
    }
}

void lf_tetris_display_myscore()
{
    int units, tens, hundreds, thousands;
    thousands = g_myscore / 1000;
    hundreds = g_myscore / 100;
    tens = g_myscore / 10 % 10;
    units = g_myscore % 10;
    lf_lcd_clear_data(0, 0, 16, 6);
    if (g_myscore > 999) {
        lf_lcd_8x16_numbers(0, 1, units);
        lf_lcd_8x16_numbers(0, 2, tens);
        lf_lcd_8x16_numbers(0, 3, hundreds);
        lf_lcd_8x16_numbers(0, 4, thousands);
    }
    else if (g_myscore > 99) {
        lf_lcd_8x16_numbers(0, 1, units);
        lf_lcd_8x16_numbers(0, 2, tens);
        lf_lcd_8x16_numbers(0, 3, hundreds);
    }
    else if (g_myscore > 9) {
        lf_lcd_8x16_numbers(0, 1, units);
        lf_lcd_8x16_numbers(0, 2, tens);
    }
    else {
        lf_lcd_8x16_numbers(0, 1, units);
    }
}

void lf_tetris_rival_score(int score)
{
    int units, tens, hundreds, thousands;
    thousands = score / 1000;
    hundreds = score / 100;
    tens = score / 10 % 10;
    units = score % 10;
    lf_lcd_clear_data(32, 0, 48, 6);
    if (score > 999) {
        lf_lcd_8x16_numbers(32, 1, units);
        lf_lcd_8x16_numbers(32, 2, tens);
        lf_lcd_8x16_numbers(32, 3, hundreds);
        lf_lcd_8x16_numbers(32, 4, thousands);
    }
    else if (score > 99) {
        lf_lcd_8x16_numbers(32, 1, units);
        lf_lcd_8x16_numbers(32, 2, tens);
        lf_lcd_8x16_numbers(32, 3, hundreds);
    }
    else if (score > 9) {
        lf_lcd_8x16_numbers(32, 1, units);
        lf_lcd_8x16_numbers(32, 2, tens);
    }
    else {
        lf_lcd_8x16_numbers(32, 1, units);
    }
}