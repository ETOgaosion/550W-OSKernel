/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <os.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <test.h>

/*struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};

struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};
struct task_info task_test_deadlock = {(uintptr_t)&test_deadlock, USER_PROCESS};
*/
static struct task_info *test_tasks[16] = {/*&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &strserver_task, &strgenerator_task,
                                           &task_test_multicore,
                                           &task_test_affinity,
                                           &task_test_deadlock
                                           */};
static int num_test_tasks = 8;

#define SHELL_BEGIN 20
#define MAXN 40
#define SCREEN_H 40

int num = 0;
char buff[MAXN + 5];
char rebuff[MAXN + 5];
// char name[10];
char argv[100];
char *new_argv[10];
void clearbuff() {
    for (int i = 0; i < num; i++) {
        buff[i] = ' ';
    }
    num = 0;
}
char path[10][20];
int pathnum = 0;
int nowshell = SHELL_BEGIN + 1;

void print_path(int func) {
    if (func == 1) {
        nowshell++;
        if (nowshell > SCREEN_H) {
            nowshell = SCREEN_H;
        }
    }
    printf("> root@UCAS_OS:~");
    if (pathnum != 0) {
        for (int i = 0; i < pathnum; i++) {
            printf("/");
            printf("%s", path[i]);
        }
    }
    printf("$ ");
}
int main() {
    // TODO:
    sys_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    print_path(0);
    int putin = -1;
    while (1) {
        if (nowshell > SCREEN_H) {
            nowshell = SCREEN_H;
        }
        do {
            putin = sys_read();
        } while (putin == -1);
        if (putin != -1) {
            if (num >= MAXN && putin != '\n' && putin != '\r') {
                sys_move_cursor(1, nowshell);
                print_path(0);
                printf("Too many input       \n");
                clearbuff();
                print_path(1);
                continue;
            }
            if (putin == '\n' || putin == '\r') {
                printf("\n");
                if (buff[0] == 'p' && buff[1] == 's' && num == 2) {
                    // show the process
                    nowshell += sys_ps();
                    if (nowshell > SCREEN_H) {
                        nowshell = SCREEN_H;
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'c' && buff[1] == 'l' && buff[2] == 'e' && buff[3] == 'a' && buff[4] == 'r' && num == 5) {
                    // clear screen
                    nowshell = SHELL_BEGIN + 1;
                    sys_clear();
                    clearbuff();
                    sys_move_cursor(1, SHELL_BEGIN);
                    printf("------------------- COMMAND -------------------\n");
                    print_path(0);
                } else if (buff[0] == 'e' && buff[1] == 'x' && buff[2] == 'e' && buff[3] == 'c' && buff[4] == ' ' && num > 5) {
                    // execute new process
                    int id = 0;
                    int i = 5;
                    int argc = 0;
                    while (i < num) {
                        int j = 0;
                        for (i; i < num && buff[i] != ' '; i++) {
                            argv[argc * 20 + j] = buff[i];
                            j++;
                        }
                        i++;
                        argv[argc * 20 + j] = '\0';
                        argc++;
                    }
                    for (int i = 0; i < argc; i++) {
                        new_argv[i] = &argv[i * 20];
                    }
                    id = sys_exec(argv, argc, new_argv);
                    nowshell++;
                    if (nowshell > SCREEN_H) {
                        nowshell = SCREEN_H;
                    }
                    if (id != 0) {
                        printf("exec process[%d]: %s\n", id, argv);
                    } else {
                        printf("exec failed, please check name\n");
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 't' && buff[1] == 'a' && buff[2] == 's' && buff[3] == 'k' && buff[4] == 's' && buff[5] == 'e' && buff[6] == 't' && buff[7] == ' ' && num > 8) {
                    if (buff[8] == '0' && buff[9] == 'x' && buff[10] >= '1' && buff[10] <= '3' && buff[11] == ' ' && buff[12] >= '0' && buff[12] <= '7' && num == 13) {
                        // ex: taskset 0x3 2
                        int mask = buff[10] - '0';
                        sys_taskset(0, mask);
                        int id = sys_spawn(test_tasks[buff[12] - '0'], NULL, AUTO_CLEANUP_ON_EXIT);
                        sys_taskset(0, 3);
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("exec process[%d]\n", id);
                        clearbuff();
                        print_path(1);
                    } else if (buff[8] == '-' && buff[9] == 'p' && buff[10] == ' ' && buff[11] == '0' && buff[12] == 'x' && buff[13] >= '1' && buff[13] <= '3' && buff[14] == ' ') {
                        int mask = buff[13] - '0';
                        int pid = 0;
                        if (buff[15] >= '0' && buff[15] <= '9' && num == 16) {
                            pid = buff[15] - '0';
                        } else if (buff[15] >= '0' && buff[15] <= '9' && buff[16] >= '0' && buff[16] <= '9' && num == 17) {
                            pid = (buff[15] - '0') * 10 + buff[16] - '0';
                        } else {
                            goto ERROR;
                        }
                        // ex: taskset -p 0x3 2
                        int ret = sys_taskset(pid, mask);
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        if (ret == 0) {
                            printf("taskset failed: please retry\n");
                        } else if (ret == 1) {
                            printf("taskset succeed\n");
                        }
                        clearbuff();
                        print_path(1);
                    }
                } else if (buff[0] == 'k' && buff[1] == 'i' && buff[2] == 'l' && buff[3] == 'l' && buff[4] == ' ' && buff[5] >= '0' && buff[5] <= '9' && num <= 7) {
                    // kill process
                    int pid;
                    if (buff[5] >= '0' && buff[5] <= '9' && num == 6) {
                        pid = buff[5] - '0';
                    } else if (buff[5] >= '0' && buff[5] <= '9' && buff[6] >= '0' && buff[6] <= '9' && num == 7) {
                        pid = (buff[5] - '0') * 10 + buff[6] - '0';
                    } else {
                        goto ERROR;
                    }
                    int ret = sys_kill(pid);
                    nowshell++;
                    if (nowshell > SCREEN_H) {
                        nowshell = SCREEN_H;
                    }
                    if (ret == 1) {
                        printf("Success: kill process[%d]\n", pid);
                    } else {
                        printf("Failed: kill process[%d]\n", pid);
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'f' && buff[1] == 's' && num == 2) {
                    // show the task can be executed
                    nowshell += sys_fs();
                    if (nowshell > SCREEN_H) {
                        nowshell = SCREEN_H;
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'm' && buff[1] == 'k' && buff[2] == 'f' && buff[3] == 's' && num == 4) {
                    // make file system
                    nowshell += sys_mkfs();
                    if (nowshell > SCREEN_H) {
                        nowshell = SCREEN_H;
                    }
                    clearbuff();
                    pathnum = 0;
                    print_path(1);
                } else if (buff[0] == 's' && buff[1] == 't' && buff[2] == 'a' && buff[3] == 't' && buff[4] == 'f' && buff[5] == 's' && num == 6) {
                    // show the state of file system
                    nowshell += sys_statfs();
                    if (nowshell > SCREEN_H) {
                        nowshell = SCREEN_H;
                    }
                    clearbuff();
                    print_path(1);
                } else if (num >= 2 && buff[0] == 'l' && buff[1] == 's') {
                    // show the files in dir
                    int i = 3;
                    int j = 0;
                    int code;
                    int func = 0;
                    if (num >= 5 && buff[2] == ' ' && buff[3] == '-' && buff[4] == 'l') {
                        func = 1;
                        i = 6;
                    }
                    while (i < num) {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    code = sys_ls(rebuff, func);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] No such dir, please try again!\n");
                    } else {
                        nowshell += code;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'c' && buff[1] == 'd' && buff[2] == ' ' && num > 3) {
                    // cd dir
                    int i = 3;
                    int j = 0;
                    int code = 0;
                    while (i < num) {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    code = sys_cd(rebuff);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] NO such dir, please try again!\n");
                    } else {
                        char name[20];
                        i = 3;
                        while (i < num) {
                            int j = 0;
                            for (i; i < num && buff[i] != ' ' && buff[i] != '/'; i++) {
                                name[j++] = buff[i];
                            }
                            name[j] = '\0';
                            i++;
                            if (strcmp(name, "..") == 0) {
                                if (pathnum != 0) {
                                    pathnum--;
                                }
                            } else if (strcmp(name, ".") != 0) {
                                strcpy(path[pathnum], name);
                                pathnum++;
                            }
                        }
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 't' && buff[1] == 'o' && buff[2] == 'u' && buff[3] == 'c' && buff[4] == 'h' && buff[5] == ' ' && num > 6) {
                    int i = 6;
                    int j = 0;
                    int code = 0;
                    while (i < num && buff[i] != '/') {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    code = sys_touch(rebuff);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] file existed, please try again!\n");
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'c' && buff[1] == 'a' && buff[2] == 't' && buff[3] == ' ' && num > 4) {
                    int i = 4;
                    int j = 0;
                    int code = 0;
                    int dirnum = 0;
                    while (i < num) {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    code = sys_cat(rebuff);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] NO such file OR no data, please try again!\n");
                    } else {
                        nowshell += code;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'm' && buff[1] == 'k' && buff[2] == 'd' && buff[3] == 'i' && buff[4] == 'r' && buff[5] == ' ' && num > 6) {
                    // mkae new dir
                    int i = 6;
                    int j = 0;
                    while (i < num && buff[i] != ' ' && buff[i] != '/') {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    int code = sys_mkdir(rebuff);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] dir existed, please try again!\n");
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'r' && buff[1] == 'm' && buff[2] == 'd' && buff[3] == 'i' && buff[4] == 'r' && buff[5] == ' ' && num > 6) {
                    // remove existed dir
                    int i = 6;
                    int j = 0;
                    while (i < num && buff[i] != ' ' && buff[i] != '/') {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    int code = sys_rmdir(rebuff);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] dir not existed, please try again!\n");
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'r' && buff[1] == 'm' && buff[2] == ' ' && num >= 3) {
                    // remove existed dir
                    int i = 3;
                    int j = 0;
                    while (i < num && buff[i] != ' ' && buff[i] != '/') {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    int code = sys_rm(rebuff);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] file not existed, please try again!\n");
                    }
                    clearbuff();
                    print_path(1);
                } else if (buff[0] == 'l' && buff[1] == 'n' && buff[2] == ' ' && num >= 3) {
                    // remove existed dir
                    int i = 3;
                    int j = 0;
                    char name[20];
                    while (i < num && buff[i] != ' ' && buff[i] != '/') {
                        name[j++] = buff[i++];
                    }
                    name[j] = '\0';
                    while (buff[i] != ' ') {
                        i++;
                    }
                    i++;
                    j = 0;
                    while (i < num && buff[i] != ' ') {
                        rebuff[j++] = buff[i++];
                    }
                    rebuff[j] = '\0';
                    int code = sys_ln(name, rebuff);
                    if (code == 0) {
                        nowshell++;
                        if (nowshell > SCREEN_H) {
                            nowshell = SCREEN_H;
                        }
                        printf("[ERROR] link failed, please try again!\n");
                    }
                    clearbuff();
                    print_path(1);
                } else {
                ERROR:
                    sys_move_cursor(1, nowshell);
                    print_path(0);
                    printf("Unknown command      \n");
                    clearbuff();
                    print_path(1);
                }
            } else if (putin == 8 || putin == 127) {
                if (num != 0) {
                    num--;
                }
                buff[num] = ' ';
                sys_move_cursor(1, nowshell);
                print_path(0);
                printf("%s", buff);
            } else {
                buff[num++] = (char)putin;
                sys_move_cursor(1, nowshell);
                print_path(0);
                printf("%s", buff);
            }
        }
    }
    return 0;
}
