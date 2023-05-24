#include <asm/common.h>
#include <drivers/screen.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/irq.h>
#include <os/pcb.h>
#include <os/smp.h>

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 40

int screen_cursor_x;
int screen_cursor_y;

int start_line = 21;
/* screen buffer */
char new_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};
char old_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};

/* cursor position */
void vt100_move_cursor(int x, int y) {
    // \033[y;xH
    printk("%c[%d;%dH", 27, y, x);
    (*current_running)->cursor_x = x;
    (*current_running)->cursor_y = y;
}

/* clear screen */
void vt100_clear() {
    // \033[2J
    printk("%c[2J", 27);
}

/* hidden cursor */
static void vt100_hidden_cursor() {
    // \033[?25l
    printk("%c[?25l", 27);
}

/* write a char */
static void screen_write_ch(char ch) {
    if (ch == '\n') {
        (*current_running)->cursor_x = 1;
        (*current_running)->cursor_y++;
        if ((*current_running)->cursor_y > SCREEN_HEIGHT) {
            k_screen_reflush();
            for (int i = start_line - 1; i < SCREEN_HEIGHT; i++) {
                for (int j = 0; j < SCREEN_WIDTH; j++) {
                    if (i == SCREEN_HEIGHT - 1) {
                        new_screen[i * SCREEN_WIDTH + j] = ' ';
                    } else {
                        new_screen[i * SCREEN_WIDTH + j] = old_screen[(i + 1) * SCREEN_WIDTH + j];
                    }
                }
            }
            (*current_running)->cursor_y = SCREEN_HEIGHT;
            /*
            (*current_running)->cursor_y = start_line;
            for(int i=start_line; i<SCREEN_HEIGHT; i++)
                for(int j=0; j<SCREEN_WIDTH; j++)
                    new_screen[i * SCREEN_WIDTH + j] = ' ';*/
        }
        // screen_cursor_x = 1;
        // screen_cursor_y++;
    } else {
        new_screen[((*current_running)->cursor_y - 1) * SCREEN_WIDTH + ((*current_running)->cursor_x - 1)] = ch;
        (*current_running)->cursor_x++;
    }
}

void init_screen(void) {
    vt100_hidden_cursor();
    vt100_clear();
}

long sys_screen_clear(void) {
    // vt100_move_cursor(0, 0);
    // vt100_clear();
    int i, j;
    for (i = 0; i < SCREEN_HEIGHT; i++) {
        for (j = 0; j < SCREEN_WIDTH; j++) {
            new_screen[i * SCREEN_WIDTH + j] = ' ';
        }
    }
    k_screen_reflush();
    return 0;
}

long sys_screen_move_cursor(int x, int y) {
    (*current_running)->cursor_x = x;
    // if(y>SCREEN_HEIGHT)
    //    (*current_running)->cursor_y = y - SCREEN_HEIGHT + start_line-1;
    // else
    (*current_running)->cursor_y = y;
    return 0;
}

long sys_screen_write(char *buff) {
    int i = 0;
    int l = k_strlen(buff);

    for (i = 0; i < l; i++) {
        screen_write_ch(buff[i]);
    }
    k_screen_reflush();
    return 0;
}

/*
 * This function is used to print the serial port when the clock
 * interrupt is triggered. However, we need to pay attention to
 * the fact that in order to speed up printing, we only refresh
 * the characters that have been modified since this time.
 */
long k_screen_reflush(void) {
    int i, j;

    int prex = (*current_running)->cursor_x;
    int prey = (*current_running)->cursor_y;
    /* here to reflush screen buffer to serial port */
    for (i = 0; i < SCREEN_HEIGHT; i++) {
        for (j = 0; j < SCREEN_WIDTH; j++) {
            /* We only print the data of the modified location. */
            if (new_screen[i * SCREEN_WIDTH + j] != old_screen[i * SCREEN_WIDTH + j]) {
                vt100_move_cursor(j + 1, i + 1);
                port_write_ch(new_screen[i * SCREEN_WIDTH + j]);
                old_screen[i * SCREEN_WIDTH + j] = new_screen[i * SCREEN_WIDTH + j];
            }
        }
    }

    /* recover cursor position */
    vt100_move_cursor(prex, prey);
    return 0;
}

void load_curpcb_cursor() {
    kernel_move_cursor((*current_running)->cursor_x, (*current_running)->cursor_y);
}

void kernel_move_cursor(int x, int y) {
    // \033[y;xH
    screen_cursor_x = x;
    screen_cursor_y = y;
}

void pcb_move_cursor(int x, int y) {
    (*current_running)->cursor_x = x;
    (*current_running)->cursor_y = y;
}
