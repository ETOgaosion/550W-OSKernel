#include <screen.h>
#include <asm/common.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/irq.h>
#include <lib/stdio.h>
#include <lib/string.h>

#define SCREEN_WIDTH    80
#define SCREEN_HEIGHT   40

int start_line = 21;
/* screen buffer */
char new_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};
char old_screen[SCREEN_HEIGHT * SCREEN_WIDTH] = {0};

/* cursor position */
void vt100_move_cursor(int x, int y)
{
    // \033[y;xH
    disable_preempt();
    printk("%c[%d;%dH", 27, y, x);
    if(get_current_cpu_id() == 0)
    {
        current_running0->cursor_x = x;
        current_running0->cursor_y = y;
    }
    else
    {
        current_running1->cursor_x = x;
        current_running1->cursor_y = y;
    }
    enable_preempt();
}

/* clear screen */
void vt100_clear()
{
    // \033[2J
    printk("%c[2J", 27);
}

/* hidden cursor */
static void vt100_hidden_cursor()
{
    // \033[?25l
    printk("%c[?25l", 27);
}

/* write a char */
static void screen_write_ch(char ch)
{
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    if (ch == '\n')
    {
        current_running->cursor_x = 1;
        current_running->cursor_y++;
        if(current_running->cursor_y > SCREEN_HEIGHT)
        {
            screen_reflush();
            for(int i=start_line - 1; i<SCREEN_HEIGHT; i++)
            {
                for(int j=0; j < SCREEN_WIDTH; j++)
                {
                    if(i==SCREEN_HEIGHT-1)
                        new_screen[i * SCREEN_WIDTH + j] = ' ';
                    else
                        new_screen[i * SCREEN_WIDTH + j] = old_screen[(i+1) * SCREEN_WIDTH + j];
                }
            }
            current_running->cursor_y = SCREEN_HEIGHT;
            /*
            current_running->cursor_y = start_line;
            for(int i=start_line; i<SCREEN_HEIGHT; i++)
                for(int j=0; j<SCREEN_WIDTH; j++)
                    new_screen[i * SCREEN_WIDTH + j] = ' ';*/
        }
            //screen_cursor_x = 1;
        //screen_cursor_y++;
    }
    else
    {
        new_screen[(current_running->cursor_y - 1) * SCREEN_WIDTH + (current_running->cursor_x - 1)] = ch;
        current_running->cursor_x++;
    }
}

void init_screen(void)
{
    vt100_hidden_cursor();
    vt100_clear();
}

void screen_clear(void)
{
    //vt100_move_cursor(0, 0);
    //vt100_clear();
    int i, j;
    for (i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (j = 0; j < SCREEN_WIDTH; j++)
        {
            new_screen[i * SCREEN_WIDTH + j] = ' ';
        }
    }
    screen_reflush();
}

void screen_move_cursor(int x, int y)
{
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    current_running->cursor_x = x;
    //if(y>SCREEN_HEIGHT)
    //    current_running->cursor_y = y - SCREEN_HEIGHT + start_line-1;
    //else 
    current_running->cursor_y = y;
    
}

void screen_write(char *buff)
{
    int i = 0;
    int l = kstrlen(buff);

    for (i = 0; i < l; i++)
    {
        screen_write_ch(buff[i]);
    }
    screen_reflush();
}

/*
 * This function is used to print the serial port when the clock
 * interrupt is triggered. However, we need to pay attention to
 * the fact that in order to speed up printing, we only refresh
 * the characters that have been modified since this time.
 */
void screen_reflush(void)
{
    int i, j;
    current_running = get_current_cpu_id() == 0 ? current_running0 : current_running1;
    int prex = current_running->cursor_x;
    int prey = current_running->cursor_y;
    /* here to reflush screen buffer to serial port */
    for (i = 0; i < SCREEN_HEIGHT; i++)
    {
        for (j = 0; j < SCREEN_WIDTH; j++)
        {
            /* We only print the data of the modified location. */
            if (new_screen[i * SCREEN_WIDTH + j] != old_screen[i * SCREEN_WIDTH + j])
            {
                vt100_move_cursor(j + 1, i + 1);
                port_write_ch(new_screen[i * SCREEN_WIDTH + j]);
                old_screen[i * SCREEN_WIDTH + j] = new_screen[i * SCREEN_WIDTH + j];
            }
        }
    }

    /* recover cursor position */
    vt100_move_cursor(prex, prey);
}
