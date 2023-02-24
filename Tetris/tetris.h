/*
 * @Descripttion: 俄罗斯方块算法
 * @version: 
 * @Author: Newt
 * @Date: 2022-09-28 18:00:14
 * @LastEditors: Newt
 * @LastEditTime: 2022-11-29 15:16:35
 */

#ifndef __TETRIS__H__
#define __TETRIS__H__

/* key control */
typedef enum {
    RIGHT,
    LEFT,
    DOWN,
    SPIN,
    NONE
} key_e;

int g_shape;      /* right now mino's shape */
int g_form;       /* right now mino's form */
int g_x;          /* mino's x location at real time */
int g_y;          /* mino's y location at real time */
int g_myscore; /* my score */
extern int g_speed;
/* Only the prefix "lf_tertris" means ur need to edit according to the situation */ 
void lf_tetris_init_game(); 
void lf_mino_draw(int shape, int form, int x, int y);
void lf_mino_clear(int shape, int form, int x, int y);
void lf_mino_preview_next(int x, int y);
void lf_mino_clear_next(int x, int y);
void lf_mino_create_new(int x, int y);
int  lf_mino_judge_legal(int shape, int form, int x, int y);
void lf_mino_record_location();
int lf_mino_move_control(key_e mode);
int  lf_tetris_check_failed();
int lf_tetris_judge_score();
void lf_tetris_welcome();
void lf_tetris_start();  
void lf_tetris_over();
void lf_tetris_display_myscore();
void lf_tetris_rival_score(int score);

#endif /* __TETRIS__H__ */