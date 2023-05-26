#pragma once

extern int screen_cursor_x;
extern int screen_cursor_y;

void vt100_move_cursor(int, int);

/* configuring screen properties */
void init_screen(void);

/* clear screen */
long sys_screen_clear(void);

/* reflush screen buffer */
long k_screen_reflush(void);

/* screen write string */
long sys_screen_write(char *buff);

/* move cursor int (x,y) */
long sys_screen_move_cursor(int x, int y);

void load_curpcb_cursor(void);

void pcb_move_cursor(int x, int y);

void kernel_move_cursor(int x, int y);