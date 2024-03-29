#include <asm/io.h>
#include <drivers/screen/screen.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/irq.h>
#include <os/pcb.h>
#include <os/smp.h>

#define SCREEN_WIDTH 150
#define SCREEN_HEIGHT 150

int screen_cursor_x;
int screen_cursor_y;

int start_line = 31;
/* screen buffer */
char new_screen[SCREEN_HEIGHT][SCREEN_WIDTH] = {0};
char old_screen[SCREEN_HEIGHT][SCREEN_WIDTH] = {0};

/* cursor position */
void vt100_move_cursor(bool force, int x, int y) {
    // \033[y;xH
    if (force) {
        k_print("%c[%d;%dH", 27, y, x);
    }
    screen_cursor_x = x;
    screen_cursor_y = y;
}

/* clear screen */
void vt100_clear() {
    // \033[2J
    // k_print("%c[2J", 27);
}

/* hidden cursor */
void vt100_hidden_cursor() {
    // \033[?25l
    // k_print("%c[?25l", 27);
}

/* write a char */
void screen_write_ch(char ch) {
    if (ch == '\n') {
        new_screen[screen_cursor_y][(screen_cursor_x - 1)] = ch;
        screen_cursor_x = 1;
        screen_cursor_y++;
        if (screen_cursor_y >= SCREEN_HEIGHT) {
            d_screen_reflush();
            for (int i = start_line; i < SCREEN_HEIGHT; i++) {
                for (int j = 0; j < SCREEN_WIDTH; j++) {
                    if (i == SCREEN_HEIGHT - 1) {
                        new_screen[i][j] = ' ';
                    } else {
                        new_screen[i][j] = old_screen[i + 1][j];
                    }
                }
            }
            screen_cursor_y = SCREEN_HEIGHT - 1;
        }
    } else if (ch == 8 || ch == 127) {
        new_screen[screen_cursor_y][(screen_cursor_x - 2)] = 0;
        vt100_move_cursor(true, screen_cursor_x - 1, screen_cursor_y);
    } else {
        new_screen[screen_cursor_y][(screen_cursor_x - 1)] = ch;
        screen_cursor_x++;
    }
}

void d_screen_init(void) {
    vt100_hidden_cursor();
    vt100_clear();
}

long sys_screen_clear(void) {
    // vt100_move_cursor(0, 0);
    // vt100_clear();
    int i, j;
    for (i = 0; i < SCREEN_HEIGHT; i++) {
        for (j = 0; j < SCREEN_WIDTH; j++) {
            new_screen[i][j] = 0;
        }
    }
    d_screen_reflush();
    return 0;
}

long sys_screen_move_cursor(int x, int y) {
    screen_cursor_x = x;
    // if(y>SCREEN_HEIGHT)
    //    screen_cursor_y = y - SCREEN_HEIGHT + start_line-1;
    // else
    screen_cursor_y = y;
    return 0;
}

long sys_screen_write_len(char *buff, int len) {
    for (int i = 0; i < len; i++) {
        k_port_write_ch(buff[i]);
    }
    return 0;
}

long sys_screen_write(char *buff) {
    sys_screen_write_len(buff, k_strlen(buff));
    return 0;
}

/*
 * This function is used to print the serial port when the clock
 * interrupt is triggered. However, we need to pay attention to
 * the fact that in order to speed up printing, we only refresh
 * the characters that have been modified since this time.
 */
long d_screen_reflush(void) {
    int i, j;

    int prex = screen_cursor_x;
    int prey = screen_cursor_y;
    /* here to reflush screen buffer to serial port */
    for (i = 0; i < SCREEN_HEIGHT; i++) {
        for (j = 0; j < SCREEN_WIDTH; j++) {
            /* We only print the data of the modified location. */
            if (new_screen[i][j] != old_screen[i][j]) {
                vt100_move_cursor(false, j + 1, i + 1);
                if (new_screen[i][j] == '\n') {
                    k_port_write_ch('\n');
                } else if (new_screen[i][j] < 20 || new_screen[i][j] > 126) {
                    k_port_write_ch(' ');
                } else {
                    k_port_write_ch(new_screen[i][j]);
                }
                old_screen[i][j] = new_screen[i][j];
            }
        }
    }

    /* recover cursor position */
    vt100_move_cursor(false, prex, prey);
    return 0;
}

void d_screen_load_curpcb_cursor() {
    d_screen_kernel_move_cursor(screen_cursor_x, screen_cursor_y);
}

void d_screen_kernel_move_cursor(int x, int y) {
    // \033[y;xH
    screen_cursor_x = x;
    screen_cursor_y = y;
}

void d_screen_pcb_move_cursor(int x, int y) {
    screen_cursor_x = x;
    screen_cursor_y = y;
}
