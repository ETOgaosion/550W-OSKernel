#pragma once

#include <common/types.h>

extern int screen_cursor_x;
extern int screen_cursor_y;

void vt100_move_cursor(bool, int, int);

/* configuring screen properties */
void d_screen_init(void);

/* clear screen */
long sys_screen_clear(void);

/* reflush screen buffer */
long d_screen_reflush(void);

/* screen write string */
long sys_screen_write_len(char *buff, int len);
long sys_screen_write(char *buff);

/* move cursor int (x,y) */
long sys_screen_move_cursor(int x, int y);

void d_screen_load_curpcb_cursor(void);

void d_screen_pcb_move_cursor(int x, int y);

void d_screen_kernel_move_cursor(int x, int y);