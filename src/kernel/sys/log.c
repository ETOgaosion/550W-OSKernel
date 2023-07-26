#include <lib/math.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <os/sys.h>

char moss_log[SYSLOG_SIZE] = {0};
int saved_console_loglevel;
int console_loglevel;
int minimum_console_loglevel;

int console_printk[4] = {
    1, /* console_loglevel */
    1, /* default_message_loglevel */
    1, /* minimum_console_loglevel */
    1, /* default_console_loglevel */
};

#define console_loglevel (console_printk[0])
#define default_message_loglevel (console_printk[1])
#define minimum_console_loglevel (console_printk[2])
#define default_console_loglevel (console_printk[3])

void k_sys_write_to_log(const char *log_msg) {
    k_strcat(moss_log, log_msg);
}

// [TODO]
long sys_syslog(int type, char *buf, int len) {
    switch (type) {
    case SYSLOG_ACTION_CLOSE:
        break;
    case SYSLOG_ACTION_OPEN:
        break;
    case SYSLOG_ACTION_READ:
    case SYSLOG_ACTION_READ_ALL:
        if (!buf || len < 0) {
            return -EINVAL;
        }
        if (!len) {
            return 0;
        }
        int actual_len = MIN(k_strlen((const char *)moss_log), len);
        k_memcpy((uint8_t *)buf, (const uint8_t *)moss_log, actual_len);
        return actual_len;
        break;
    case SYSLOG_ACTION_CLEAR:
        k_bzero((uint8_t *)moss_log, sizeof(moss_log));
        break;
    /* Disable logging to console */
    case SYSLOG_ACTION_CONSOLE_OFF:
        if (saved_console_loglevel == LOGLEVEL_DEFAULT) {
            saved_console_loglevel = console_loglevel;
        }
        console_loglevel = minimum_console_loglevel;
        break;
    /* Enable logging to console */
    case SYSLOG_ACTION_CONSOLE_ON:
        if (saved_console_loglevel != LOGLEVEL_DEFAULT) {
            console_loglevel = saved_console_loglevel;
            saved_console_loglevel = LOGLEVEL_DEFAULT;
        }
        break;
    /* Set level of messages printed to console */
    case SYSLOG_ACTION_CONSOLE_LEVEL:
        if (len < 1 || len > 8) {
            return -EINVAL;
        }
        if (len < minimum_console_loglevel) {
            len = minimum_console_loglevel;
        }
        console_loglevel = len;
        /* Implicitly re-enable logging to console */
        saved_console_loglevel = LOGLEVEL_DEFAULT;
        break;
    case SYSLOG_ACTION_SIZE_BUFFER:
        return SYSLOG_SIZE;
        break;

    default:
        return -EINVAL;
        break;
    }
    return 0;
}