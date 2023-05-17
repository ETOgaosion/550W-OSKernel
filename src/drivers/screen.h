#pragma once

extern int screen_cursor_x;
extern int screen_cursor_y;

void vt100_move_cursor(int, int);

/* configuring screen properties */
void init_screen(void);

/* clear screen */
void sys_screen_clear(void);

/* reflush screen buffer */
void sys_screen_reflush(void);

/* screen write string */
void sys_screen_write(char *buff);

/* move cursor int (x,y) */
void sys_screen_move_cursor(int x, int y);
