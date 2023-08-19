#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FINAL

#define SHELL_BEGIN 148
#define SHELL_ARG_MAX_NUM 5
#define SHELL_ARG_MAX_LENGTH 20
#define SHELL_INPUT_MAX_WORDS 100
#define SHELL_CMD_MAX_LENGTH 2 * 8

/* clang-format off */
#define BEGIN printf("\n========================== MOSS ==========================")
/* clang-format on */

#define HELP 0
#define EXEC 1
#define EXECVE 2
#define KILL 3
#define PS 4
#define CLEAR 5
// #define TASKSET 6
// #define MKFS 7
// #define STATFS 8
// #define MKDIR 9
// #define RMDIR 10
// #define CD 11
// #define LS 12
// #define PWD 13
// #define TOUCH 14
// #define CAT 15
// #define RM 16
// #define LN 17

typedef int (*function)(int argc, char *argv[]);

#ifndef FINAL
#define CURRENT_TASK_NUM 32
// #define CURRENT_TASK_NUM 1

/* clang-format off */
char *task_names[CURRENT_TASK_NUM] = {
    "getppid", "sleep", "gettimeofday", "times", "getpid", "uname", "brk", 
    "wait", "fork", "waitpid", "clone", "yield", "exit",
    "openat", "dup2", "write", "unlink", "getdents", "dup", "mount", "umount", "fstat", 
    "getcwd", "close", "read", "open", "mkdir_",
    "mmap", "munmap", "pipe", "chdir", "execve",
};
/* clang-format on */

// char *task_names[CURRENT_TASK_NUM] = {"execve"};
#endif

void panic(char *error);
static int shell_help(int argc, char *argv[]);
static int shell_exec(int argc, char *argv[]);
static int shell_execve(int argc, char *argv[]);
static int shell_kill(int argc, char *argv[]);
static int shell_ps(int argc, char *argv[]);
static int shell_clear(int argc, char *argv[]);
// static int shell_taskset(int argc, char *argv[]);
// static int shell_mkfs(int argc, char *argv[]);
// static int shell_statfs(int argc, char *argv[]);
// static int shell_mkdir(int argc, char *argv[]);
// static int shell_rmdir(int argc, char *argv[]);
// static int shell_cd(int argc, char *argv[]);
// static int shell_ls(int argc, char *argv[]);
// static int shell_pwd(int argc, char *argv[]);
// static int shell_touch(int argc, char *argv[]);
// static int shell_cat(int argc, char *argv[]);
// static int shell_rm(int argc, char *argv[]);
// static int shell_ln(int argc, char *argv[]);

typedef struct cmds {
    char *cmd_full_name;
    char *cmd_alias;
    char *description;
    char *format;
    function handler;
    int max_arg_num;
    int min_arg_num;
} cmds_t;

/* clang-format off */
cmds_t cmd_table[] = {
    {"help", "h", "Print description of command [cmd] or all supported commands(no args or error cmd)", "help ([cmd])", &shell_help, SHELL_ARG_MAX_NUM, 0},
    {"exec", "spawn", "Execute task name in taskset", "exec [filename] [&]", &shell_exec, 1, 1},
    {"execve", "eve", "Execute task name in taskset with args and env definations", "execve [filename] [-a [argv]] [-e [env]] [&]", &shell_execve, SHELL_ARG_MAX_NUM, 1}, 
    {"kill", "k", "Kill process [pid](start from 1)", "kill [pid]", &shell_kill, 1, 1}, 
    {"ps", "ps", "Display all process", "ps", &shell_ps, 0, 0}, 
    {"clear", "clr", "Clear the screen", "clear", &shell_clear, 0, 0},
    // {"taskset","ts","Start a task's or set some task's running core","taskset [mask] [taskid]/taskset -p [mask] [taskid]",&shell_taskset,3,3},
    // {"mkfs","mkfs", "Initial file system", "mkfs", &shell_mkfs, 0,0},
    // {"statfs", "stfs", "Show info of file system", "statfs", &shell_statfs, 0,0},
    // {"mkdir", "md", "Create a directory", "mkdir [name]", &shell_mkdir, 1,1},
    // {"rmdir", "rmd", "Remove a directory", "rmdir [name]", &shell_rmdir, 1,1},
    // {"cd", "c", "Change a directory", "cd [name]", &shell_cd, 1,1},
    // {"ls", "l", "Show content of directory", "ls [-option] [name]", &shell_ls, 2,0},
    // {"pwd", "p", "Show current path", "pwd", &shell_pwd, 0,0},
    // {"touch", "t", "Create a file", "touch [name]", &shell_touch, 1,1},
    // {"cat", "cat", "Print content of file to console", "cat [name]", &shell_cat, 1,1},
    // {"rm", "rm", "Remove a file", "rm [name]", &shell_rm, 1,1},
    // {"ln", "ln", "Link file to another directory", "ln [-s] <path1> <path2>", &shell_ln, 3,2}
};
/* clang-format on */

#define SUPPORTED_CMD_NUM sizeof(cmd_table) / sizeof(cmds_t)

char cmd_not_found[] = "command not found";
char arg_num_error[] = "arg number or format doesn't match, or we cannot find what you ask";
char cmd_error[] = "there are errors during the execution of cmd, please check and re-enter";

int arg_split(char *args, char *arg_out[]) {
    int arg_idx = 0;
    while ((args = strtok(arg_out[arg_idx++], args, ' ', 100)) != NULL && arg_idx < 10)
        ;
    return arg_idx;
}

static int shell_help(int argc, char *argv[]) {
    if (argc > 0) {
        for (int i = 0; i < argc; i++) {
            char *cmd = argv[i];
            for (int i = 0; i < SUPPORTED_CMD_NUM; i++) {
                if (strcmp(cmd, cmd_table[i].cmd_full_name) == 0 || strcmp(cmd, cmd_table[i].cmd_alias) == 0) {
                    printf("\ncommand: %s, alias: %s, discription: %s, format: %s, max arg number: %d;", cmd_table[i].cmd_full_name, cmd_table[i].cmd_alias, cmd_table[i].description, cmd_table[i].format, cmd_table[i].max_arg_num);
                    return 0;
                }
            }
            printf("\n%s", cmd_not_found);
            return -1;
        }

    } else {
        printf("\nall commands are listed below:");
        for (int i = 0; i < SUPPORTED_CMD_NUM; i++) {
            printf("\ncommand: %s, alias: %s, discription: %s, format: %s, max arg number: %d;", cmd_table[i].cmd_full_name, cmd_table[i].cmd_alias, cmd_table[i].description, cmd_table[i].format, cmd_table[i].max_arg_num);
        }
    }
    return 0;
}

// static int shell_echo(int argc, char *argv[]) {
//     printf("\n");
//     for (int i = 0; i < argc; i++) {
//         printf("%s", argv[i]);
//     }
//     return 0;
// }

// static int shell_echo_once(const char *argv) {
//     printf("\n%s", argv);
//     return 0;
// }

static int shell_exec(int argc, char *argv[]) {
    char *first_arg = argv[0];
    printf("\ntask[%s] will be started soon!", first_arg);
    if (argc == 1) {
        int pid = spawn(first_arg);
        int result = 0;
        waitpid(pid, &result, 0);
        return result;
    }
    int argp = 0, envp = 0;
    for (int i = 1; i < argc; i++) {
        if (argp && envp) {
            break;
        }
        if (!argp && strcmp(argv[i], "-a") == 0) {
            argp = i + 1;
        }
        if (!envp && strcmp(argv[i], "-e") == 0) {
            envp = i + 1;
        }
    }
    char arg[SHELL_ARG_MAX_NUM][SHELL_ARG_MAX_LENGTH] = {0};
    char env[SHELL_ARG_MAX_NUM][SHELL_ARG_MAX_LENGTH] = {0};
    int i = argp;
    for (i = argp; i < envp - 1 && i < argc; i++) {
        memcpy((uint8_t *)arg[i - argp], (uint8_t *)argv[i], strlen((const char *)argv[i]));
    }
    i = envp;
    for (i = envp; i < argp - 1 && i < argc; i++) {
        memcpy((uint8_t *)arg[i - envp], (uint8_t *)argv[i], strlen((const char *)argv[i]));
    }
    int pid = exec(first_arg, (char *const *)arg, (char *const *)env);
    if (strcmp((const char *)argv[argc - 1], "&") != 0) {
        int result = 0;
        waitpid(pid, &result, 0);
        return result;
    }
    return 0;
}

static int shell_execve(int argc, char *argv[]) {
    char *first_arg = (char *)argv;
    printf("\ntask[%s] will be started soon!", first_arg);
    if (argc == 1) {
        int pid = spawn(first_arg);
        int result = 0;
        waitpid(pid, &result, 0);
        return result;
    }
    pid_t pid = fork();
    if (pid) {
        int result = 0;
        waitpid(pid, &result, 0);
        return result;
    } else {
        int argp = 0, envp = 0;
        for (int i = 1; i < argc; i++) {
            if (argp && envp) {
                break;
            }
            if (!argp && strcmp(argv[i], "-a") == 0) {
                argp = i + 1;
            }
            if (!envp && strcmp(argv[i], "-e") == 0) {
                envp = i + 1;
            }
        }
        char arg[SHELL_ARG_MAX_NUM][SHELL_ARG_MAX_LENGTH] = {0};
        char env[SHELL_ARG_MAX_NUM][SHELL_ARG_MAX_LENGTH] = {0};
        int i = argp;
        for (i = argp; i < envp - 1 && i < argc; i++) {
            memcpy((uint8_t *)arg[i - argp], (uint8_t *)argv[i], strlen((const char *)argv[i]));
        }
        i = envp;
        for (i = envp; i < argp - 1 && i < argc; i++) {
            memcpy((uint8_t *)arg[i - envp], (uint8_t *)argv[i], strlen((const char *)argv[i]));
        }
        execve(first_arg, (char *const *)arg, (char *const *)env);
        return 0;
    }
}

static int shell_kill(int argc, char *argv[]) {
    if (argc < cmd_table[KILL].min_arg_num) {
        panic(arg_num_error);
        return -1;
    }
    char *first_arg = (char *)argv;
    int pid = strtol(first_arg, NULL, 10);
    printf("\ntask[%d] will be killed soon!", pid);
    sys_kill(pid);
    return 0;
}

static int shell_ps(int argc, char *argv[]) {
    sys_process_show();
    return 0;
}

static int shell_clear(int argc, char *argv[]) {
    screen_clear();
    BEGIN;
    return 0;
}

// static int shell_taskset(int argc, char *argv[])
// {
//     if(argc < cmd_table[TASKSET].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     int mask, pid;
//     char *first_arg = (char *)argv;
//     char *second_arg = (char *)argv + SHELL_ARG_MAX_LENGTH;
//     char *third_arg = (char *)argv + 2*SHELL_ARG_MAX_LENGTH;
//     if(strcmp(first_arg,"-p") == 0){
//         mask = strtol(second_arg,NULL,16);
//         pid = strtol(third_arg,NULL,10);
//         printf("\ntask[%d] mask will be set soon!",pid);
//         return 0;
//     }
//     else{
//         mask = strtol(first_arg,NULL,16);
//         if(shell_exec(argc - 1,argv + strlen(first_arg)) < 0){
//             return -1;
//         }
//         printf("\ntask[%d] mask will be set soon!",pid);
//         return 0;
//     }
// }

// static int shell_mkfs(int argc, char *argv[])
// {
//     int option = 0;
//     if(argc != 0){
//         option = strtol((char *)argv,NULL,16);
//     }
//     return mkfs(option);
// }

// static int shell_statfs(int argc, char *argv[])
// {
//     return statfs();
// }

// static int shell_mkdir(int argc, char *argv[])
// {
//     if(argc < cmd_table[MKDIR].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *dirname = (char *)argv;
//     return mkdir(dirname);
// }

// static int shell_rmdir(int argc, char *argv[])
// {
//     if(argc < cmd_table[RMDIR].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *dirname = (char *)argv;
//     return rmdir(dirname);
// }

// static int shell_cd(int argc, char *argv[])
// {
//     if(argc < cmd_table[CD].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *dirname = (char *)argv;
//     return cd(dirname);
// }

// static int shell_ls(int argc, char *argv[])
// {
//     char filename[SHELL_ARG_MAX_LENGTH] = {0};
//     int option = 0;
//     if(argc != 1 && argc != 2){
//         option = -1;
//     }
//     char *first_arg = (char *)argv;
//     if(argc == 1){
//         if(first_arg[0] == '-'){
//             option = first_arg[1] - 'a';
//         }
//         else{
//             memcpy((uint8_t *)filename, first_arg, SHELL_ARG_MAX_LENGTH);
//         }
//     }
//     char *second_arg = (char *)argv + SHELL_ARG_MAX_LENGTH;
//     if(argc == 2){
//         if(first_arg[0] != '-'){
//             panic(arg_num_error);
//         }
//         else{
//             option = first_arg[1] - 'a';
//             memcpy((uint8_t *)filename, second_arg, SHELL_ARG_MAX_LENGTH);
//         }
//     }
//     return ls(filename,option);
// }

// static int shell_pwd(int argc, char *argv[])
// {
//     if(argc < cmd_table[PWD].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char filename[SHELL_ARG_MAX_LENGTH];
//     pwd(filename);
//     printf("%s\n",filename);
//     return 0;
// }

// static int shell_touch(int argc, char *argv[])
// {
//     if(argc < cmd_table[TOUCH].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *filename = (char *)argv;
//     return touch(filename);
// }

// static int shell_cat(int argc, char *argv[])
// {
//     if(argc < cmd_table[CAT].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *filename = (char *)argv;
//     return cat(filename);
// }

// static int shell_rm(int argc, char *argv[])
// {
//     if(argc < cmd_table[RM].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *filename = (char *)argv;
//     return frm(filename);
// }

// static int shell_ln(int argc, char *argv[])
// {
//     if(argc < cmd_table[RM].min_arg_num){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *first_arg = (char *)argv;
//     char *second_arg = (char *)argv + SHELL_ARG_MAX_LENGTH;
//     if(argc == 2){
//         return ln(0,first_arg,second_arg);
//     }
//     if(strcmp(first_arg,"-s") || (strcmp(first_arg,"-h"))){
//         panic(arg_num_error);
//         return -1;
//     }
//     char *third_arg = (char *)argv + 2 * SHELL_ARG_MAX_LENGTH;
//     return ln(first_arg[1] - 'h' > 0 ? 1 : 0, second_arg, third_arg);
// }

// #define DEBUG_WITHOUT_INIT_FS

#ifdef FINAL
static void time_test(bool execute) {
    if (!execute) {
        return;
    }
    printf("\nrun time-test\n");
    int pid = exec("time-test", NULL, NULL);
    int res = 0;
    waitpid(pid, &res, 0);
}

static void busybox_test(bool execute) {
    if (!execute) {
        return;
    }
    printf("run busybox_testcode.sh\n");
    /* clang-format off */
    int busybox_arg_nums[] = {
        2, 3, 3, 2, 1,
        1, 1, 1, 2, 1,
        1, 4, 1, 1, 2,
        1, 1, 2, 1, 1,
        1, 1, 2, 1, 2,
        2, 2, 3, 2, 4,
        2, 2, 2, 3, 2,
        3, 3, 3, 3, 3,
        3, 2, 2, 2, 2,
        4, 2, 2, 2, 3,
        2, 3, 3, 2, 3
    };

    char *busybox_cmd_string[] = {
        "echo \"#### independent command test\"", "ash -c exit", "sh -c exit", "basename /aaa/bbb", "cal", "clear", "date ", "df", "dirname /aaa/bbb", "dmesg ", "du", "expr 1 + 1", "false", "true", "which ls", "uname", "uptime", "printf \"abc\\n\"", "ps", "pwd", "free", "hwclock", "kill 10", "ls", "sleep 1", "echo \"#### file opration test\"", "touch test.txt", "echo \"hello world\" > test.txt", "cat test.txt", "cut -c 3 test.txt", "od test.txt", "head test.txt", "tail test.txt ", "hexdump -C test.txt ", "md5sum test.txt", "echo \"ccccccc\" >> test.txt", "echo \"bbbbbbb\" >> test.txt", "echo \"aaaaaaa\" >> test.txt", "echo \"2222222\" >> test.txt", "echo \"1111111\" >> test.txt", "echo \"bbbbbbb\" >> test.txt", "sort test.txt | ./busybox uniq", "stat test.txt", "strings test.txt ", "wc test.txt", "[ -f test.txt ]", "more test.txt", "rm test.txt", "mkdir test_dir", "mv test_dir test", "rmdir test", "grep hello busybox_cmd.txt", "cp busybox_cmd.txt busybox_cmd.bak", "rm busybox_cmd.bak",
        "find -name \"busybox_cmd.txt\"",
    };
    char *busybox_arg1[][2] = {{"cal"}, {"clear"}, {"date"}, {"df"}, {"dmesg"}, {"du"}, {"false"}, {"true"}, {"uname"}, {"uptime"}, {"ps"}, {"pwd"}, {"free"}, {"hwclock"}, {"ls"}};
    char *busybox_arg2[][3] = {
        {"echo", "#### independent command test"}, {"basename", "/aaa/bbb"}, {"dirname", "/aaa/bbb"}, {"which", "ls"}, {"printf", "abc\n"}, {"kill", "10"}, {"sleep", "1"}, {"echo", "#### file operation test"}, {"touch", "test.txt"}, {"cat", "test.txt"}, {"od", "test.txt"}, {"head", "test.txt"}, {"tail", "test.txt"}, {"md5sum", "test.txt"}, {"sort", "test.txt"}, {"stat", "test.txt"}, {"strings", "test.txt"}, {"wc", "test.txt"}, {"more", "test.txt"}, {"rm", "test.txt"}, {"mkdir", "test_dir"}, {"rmdir", "test"}, {"rm", "busybox_cmd.bak"},
    };
    char *busybox_arg3[][4] = {{"ash", "-c", "exit"},
                               {"sh", "-c", "exit"},
                               {"sh", "-c", "echo \"hello world\" > test.txt"},
                               {"hexdump", "-C", "test.txt"},
                               {"sh", "-c", "echo ccccccc >> test.txt"},
                               {"sh", "-c", "echo bbbbbbb >> test.txt"},
                               {"sh", "-c", "echo aaaaaaa >> test.txt"},
                               {"sh", "-c", "echo 2222222 >> test.txt"},
                               {"sh", "-c", "echo 1111111 >> test.txt"},
                               {"sh", "-c", "echo bbbbbbb >> test.txt"},
                               // {"sh", "-c", "sort test.txt | ./busybox unique"},
                               {"mv", "test_dir", "test"},
                               {"grep", "hello", "busybox_cmd.txt"},
                               {"cp", "busybox_cmd.txt", "busybox_cmd.bak"},
                               {"find", "-name", "busybox_cmd.txt"}};
    char *busybox_arg4[][5] = {
        {"expr", "1", "+", "1"},
        {"cut", "-c", "3", "test.txt"},
        {"[", "-f", "test.txt", "]"},
    };
    /* clang-format on */
    int arg1_ptr = 0, arg2_ptr = 0, arg3_ptr = 0, arg4_ptr = 0;
    int pid = 0, res = 0;
    for (int i = 0; i < 55; i++) {
        if (i == 14) {
            printf("ls is built-in command\n");
            arg2_ptr++;
            goto print_res;
        }
        if (i == 18) {
            sys_process_show();
            arg1_ptr++;
            goto print_res;
        }
        if (i == 7 || i == 10 || i == 23) {
            arg1_ptr++;
            goto print_res;
        }
        if (busybox_arg_nums[i] == 1) {
            pid = exec("busybox", (char *const *)busybox_arg1[arg1_ptr], NULL);
            res = 0;
            waitpid(pid, &res, 0);
            arg1_ptr++;
        } else if (busybox_arg_nums[i] == 2) {
            pid = exec("busybox", (char *const *)busybox_arg2[arg2_ptr], NULL);
            res = 0;
            waitpid(pid, &res, 0);
            arg2_ptr++;
        } else if (busybox_arg_nums[i] == 3) {
            pid = exec("busybox", (char *const *)busybox_arg3[arg3_ptr], NULL);
            res = 0;
            waitpid(pid, &res, 0);
            arg3_ptr++;
        } else if (busybox_arg_nums[i] == 4) {
            pid = exec("busybox", (char *const *)busybox_arg4[arg4_ptr], NULL);
            res = 0;
            waitpid(pid, &res, 0);
            arg4_ptr++;
        }

    print_res:
        printf("testcase busybox %s ", busybox_cmd_string[i]);
        if (!(res >> 8) || !strcmp((const char *)busybox_cmd_string[i], "false")) {
            printf("success\n");
        } else {
            printf("fail\n");
        }
    }
}

static void lua_test(bool execute) {
    if (!execute) {
        return;
    }
    printf("run lua_testcode.sh\n");
    char *lua_args[][3] = {{"./lua", "date.lua"}, {"./lua", "file_io.lua"}, {"./lua", "max_min.lua"}, {"./lua", "random.lua"}, {"./lua", "remove.lua"}, {"./lua", "round_num.lua"}, {"./lua", "sin30.lua"}, {"./lua", "sort.lua"}, {"./lua", "strings.lua"}};
    int pid = 0, res = 0;
    for (int i = 0; i < 9; i++) {
        pid = exec("lua", (char *const *)lua_args[i], NULL);
        res = 0;
        waitpid(pid, &res, 0);
        printf("testcase lua %s", lua_args[i][1]);
        if (!(res >> 8)) {
            printf(" success\n");
        } else {
            printf(" fail\n");
        }
    }
}

static void iozone_test(bool execute) {
    if (!execute) {
        return;
    }
    printf("run iozone_testcode.sh");
    printf("iozone automatic measurements\n");
    char *iozone_arg1[] = {"-a", "-r", "1k", "-s", "4m"};
    int pid = exec("iozone", iozone_arg1, NULL);
    int res = 0;
    waitpid(pid, &res, 0);
    char *iozone_args[][11] = {
        {"-t", "4", "-i", "0", "-i", "1", "-r", "1k", "-s", "1m"}, {"-t", "4", "-i", "0", "-i", "2", "-r", "1k", "-s", "1m"}, {"-t", "4", "-i", "0", "-i", "3", "-r", "1k", "-s", "1m"}, {"-t", "4", "-i", "0", "-i", "5", "-r", "1k", "-s", "1m"}, {"-t", "4", "-i", "0", "-i", "7", "-r", "1k", "-s", "1m"}, {"-t", "4", "-i", "0", "-i", "10", "-r", "1k", "-s", "1m"}, {"-t", "4", "-i", "0", "-i", "12", "-r", "1k", "-s", "1m"},
    };
    char *measures[] = {
        "write/read", "random-read", "read-backwards", "stride-read", "fwrite/fread", "pwrite/pread", "pwritev/preadv",
    };
    for (int i = 0; i < 7; i++) {
        printf("iozone throughput %s measurements\n", measures[i]);
        pid = exec("iozone", (char *const *)iozone_args[i], NULL);
        res = 0;
        waitpid(pid, &res, 0);
    }
}

static void libc_test(bool execute) {
    if (!execute) {
        return;
    }
    printf("run libctest_testcode.sh\n");
    int pid = 0, res = 0;
    char *run_static_args[][5] = {
        {"./runtest.exe", "-w", "entry-static.exe", "argv"},
        {"./runtest.exe", "-w", "entry-static.exe", "basename"},
        {"./runtest.exe", "-w", "entry-static.exe", "clocale_mbfuncs"},
        {"./runtest.exe", "-w", "entry-static.exe", "clock_gettime"},
        {"./runtest.exe", "-w", "entry-static.exe", "crypt"},
        {"./runtest.exe", "-w", "entry-static.exe", "dirname"},
        {"./runtest.exe", "-w", "entry-static.exe", "env"},
        {"./runtest.exe", "-w", "entry-static.exe", "fdopen"},
        {"./runtest.exe", "-w", "entry-static.exe", "fnmatch"},
        {"./runtest.exe", "-w", "entry-static.exe", "fscanf"},
        {"./runtest.exe", "-w", "entry-static.exe", "fwscanf"},
        {"./runtest.exe", "-w", "entry-static.exe", "iconv_open"},
        {"./runtest.exe", "-w", "entry-static.exe", "inet_pton"},
        {"./runtest.exe", "-w", "entry-static.exe", "mbc"},
        {"./runtest.exe", "-w", "entry-static.exe", "memstream"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_cancel_points"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_cancel"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_cond"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_tsd"},
        {"./runtest.exe", "-w", "entry-static.exe", "qsort"},
        {"./runtest.exe", "-w", "entry-static.exe", "random"},
        {"./runtest.exe", "-w", "entry-static.exe", "search_hsearch"},
        {"./runtest.exe", "-w", "entry-static.exe", "search_insque"},
        {"./runtest.exe", "-w", "entry-static.exe", "search_lsearch"},
        {"./runtest.exe", "-w", "entry-static.exe", "search_tsearch"},
        {"./runtest.exe", "-w", "entry-static.exe", "setjmp"},
        {"./runtest.exe", "-w", "entry-static.exe", "snprintf"},
        {"./runtest.exe", "-w", "entry-static.exe", "socket"},
        {"./runtest.exe", "-w", "entry-static.exe", "sscanf"},
        {"./runtest.exe", "-w", "entry-static.exe", "sscanf_long"},
        {"./runtest.exe", "-w", "entry-static.exe", "stat"},
        {"./runtest.exe", "-w", "entry-static.exe", "strftime"},
        {"./runtest.exe", "-w", "entry-static.exe", "string"},
        {"./runtest.exe", "-w", "entry-static.exe", "string_memcpy"},
        {"./runtest.exe", "-w", "entry-static.exe", "string_memmem"},
        {"./runtest.exe", "-w", "entry-static.exe", "string_memset"},
        {"./runtest.exe", "-w", "entry-static.exe", "string_strchr"},
        {"./runtest.exe", "-w", "entry-static.exe", "string_strcspn"},
        {"./runtest.exe", "-w", "entry-static.exe", "string_strstr"},
        {"./runtest.exe", "-w", "entry-static.exe", "strptime"},
        {"./runtest.exe", "-w", "entry-static.exe", "strtod"},
        {"./runtest.exe", "-w", "entry-static.exe", "strtod_simple"},
        {"./runtest.exe", "-w", "entry-static.exe", "strtof"},
        {"./runtest.exe", "-w", "entry-static.exe", "strtol"},
        {"./runtest.exe", "-w", "entry-static.exe", "strtold"},
        {"./runtest.exe", "-w", "entry-static.exe", "swprintf"},
        {"./runtest.exe", "-w", "entry-static.exe", "tgmath"},
        {"./runtest.exe", "-w", "entry-static.exe", "time"},
        {"./runtest.exe", "-w", "entry-static.exe", "tls_align"},
        {"./runtest.exe", "-w", "entry-static.exe", "udiv"},
        {"./runtest.exe", "-w", "entry-static.exe", "ungetc"},
        {"./runtest.exe", "-w", "entry-static.exe", "utime"},
        {"./runtest.exe", "-w", "entry-static.exe", "wcsstr"},
        {"./runtest.exe", "-w", "entry-static.exe", "wcstol"},
        {"./runtest.exe", "-w", "entry-static.exe", "pleval"},
        {"./runtest.exe", "-w", "entry-static.exe", "daemon_failure"},
        {"./runtest.exe", "-w", "entry-static.exe", "dn_expand_empty"},
        {"./runtest.exe", "-w", "entry-static.exe", "dn_expand_ptr_0"},
        {"./runtest.exe", "-w", "entry-static.exe", "fflush_exit"},
        {"./runtest.exe", "-w", "entry-static.exe", "fgets_eof"},
        {"./runtest.exe", "-w", "entry-static.exe", "fgetwc_buffering"},
        {"./runtest.exe", "-w", "entry-static.exe", "fpclassify_invalid_ld80"},
        {"./runtest.exe", "-w", "entry-static.exe", "ftello_unflushed_append"},
        {"./runtest.exe", "-w", "entry-static.exe", "getpwnam_r_crash"},
        {"./runtest.exe", "-w", "entry-static.exe", "getpwnam_r_errno"},
        {"./runtest.exe", "-w", "entry-static.exe", "iconv_roundtrips"},
        {"./runtest.exe", "-w", "entry-static.exe", "inet_ntop_v4mapped"},
        {"./runtest.exe", "-w", "entry-static.exe", "inet_pton_empty_last_field"},
        {"./runtest.exe", "-w", "entry-static.exe", "iswspace_null"},
        {"./runtest.exe", "-w", "entry-static.exe", "lrand48_signextend"},
        {"./runtest.exe", "-w", "entry-static.exe", "lseek_large"},
        {"./runtest.exe", "-w", "entry-static.exe", "malloc_0"},
        {"./runtest.exe", "-w", "entry-static.exe", "mbsrtowcs_overflow"},
        {"./runtest.exe", "-w", "entry-static.exe", "memmem_oob_read"},
        {"./runtest.exe", "-w", "entry-static.exe", "memmem_oob"},
        {"./runtest.exe", "-w", "entry-static.exe", "mkdtemp_failure"},
        {"./runtest.exe", "-w", "entry-static.exe", "mkstemp_failure"},
        {"./runtest.exe", "-w", "entry-static.exe", "printf_1e9_oob"},
        {"./runtest.exe", "-w", "entry-static.exe", "printf_fmt_g_round"},
        {"./runtest.exe", "-w", "entry-static.exe", "printf_fmt_g_zeros"},
        {"./runtest.exe", "-w", "entry-static.exe", "printf_fmt_n"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_robust_detach"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_cancel_sem_wait"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_cond_smasher"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_condattr_setclock"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_exit_cancel"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_once_deadlock"},
        {"./runtest.exe", "-w", "entry-static.exe", "pthread_rwlock_ebusy"},
        {"./runtest.exe", "-w", "entry-static.exe", "putenv_doublefree"},
        {"./runtest.exe", "-w", "entry-static.exe", "regex_backref_0"},
        {"./runtest.exe", "-w", "entry-static.exe", "regex_bracket_icase"},
        {"./runtest.exe", "-w", "entry-static.exe", "regex_ere_backref"},
        {"./runtest.exe", "-w", "entry-static.exe", "regex_escaped_high_byte"},
        {"./runtest.exe", "-w", "entry-static.exe", "regex_negated_range"},
        {"./runtest.exe", "-w", "entry-static.exe", "regexec_nosub"},
        {"./runtest.exe", "-w", "entry-static.exe", "rewind_clear_error"},
        {"./runtest.exe", "-w", "entry-static.exe", "rlimit_open_files"},
        {"./runtest.exe", "-w", "entry-static.exe", "scanf_bytes_consumed"},
        {"./runtest.exe", "-w", "entry-static.exe", "scanf_match_literal_eof"},
        {"./runtest.exe", "-w", "entry-static.exe", "scanf_nullbyte_char"},
        {"./runtest.exe", "-w", "entry-static.exe", "setvbuf_unget"},
        {"./runtest.exe", "-w", "entry-static.exe", "sigprocmask_internal"},
        {"./runtest.exe", "-w", "entry-static.exe", "sscanf_eof"},
        {"./runtest.exe", "-w", "entry-static.exe", "statvfs"},
        {"./runtest.exe", "-w", "entry-static.exe", "strverscmp"},
        {"./runtest.exe", "-w", "entry-static.exe", "syscall_sign_extend"},
        {"./runtest.exe", "-w", "entry-static.exe", "uselocale_0"},
        {"./runtest.exe", "-w", "entry-static.exe", "wcsncpy_read_overflow"},
        {"./runtest.exe", "-w", "entry-static.exe", "wcsstr_false_negative"},
    };
    for (int i = 0; i < 109; i++) {
        pid = exec("runtest.exe", (char *const *)run_static_args[i], NULL);
        res = 0;
        waitpid(pid, &res, 0);
    }
    char *run_dynamic_args[][5] = {
        {"./runtest.exe", "-w", "entry-dynamic.exe", "argv"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "basename"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "clocale_mbfuncs"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "clock_gettime"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "crypt"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "dirname"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "dlopen"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "env"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fdopen"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fnmatch"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fscanf"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fwscanf"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "iconv_open"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "inet_pton"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "mbc"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "memstream"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_cancel_points"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_cancel"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_cond"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_tsd"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "qsort"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "random"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "search_hsearch"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "search_insque"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "search_lsearch"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "search_tsearch"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "sem_init"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "setjmp"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "snprintf"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "socket"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "sscanf"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "sscanf_long"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "stat"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strftime"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "string"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "string_memcpy"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "string_memmem"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "string_memset"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "string_strchr"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "string_strcspn"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "string_strstr"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strptime"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strtod"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strtod_simple"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strtof"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strtol"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strtold"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "swprintf"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "tgmath"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "time"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "tls_init"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "tls_local_exec"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "udiv"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "ungetc"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "utime"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "wcsstr"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "wcstol"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "daemon_failure"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "dn_expand_empty"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "dn_expand_ptr_0"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fflush_exit"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fgets_eof"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fgetwc_buffering"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "fpclassify_invalid_ld80"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "ftello_unflushed_append"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "getpwnam_r_crash"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "getpwnam_r_errno"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "iconv_roundtrips"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "inet_ntop_v4mapped"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "inet_pton_empty_last_field"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "iswspace_null"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "lrand48_signextend"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "lseek_large"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "malloc_0"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "mbsrtowcs_overflow"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "memmem_oob_read"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "memmem_oob"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "mkdtemp_failure"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "mkstemp_failure"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "printf_1e9_oob"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "printf_fmt_g_round"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "printf_fmt_g_zeros"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "printf_fmt_n"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_robust_detach"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_cond_smasher"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_condattr_setclock"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_exit_cancel"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_once_deadlock"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "pthread_rwlock_ebusy"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "putenv_doublefree"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "regex_backref_0"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "regex_bracket_icase"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "regex_ere_backref"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "regex_escaped_high_byte"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "regex_negated_range"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "regexec_nosub"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "rewind_clear_error"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "rlimit_open_files"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "scanf_bytes_consumed"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "scanf_match_literal_eof"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "scanf_nullbyte_char"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "setvbuf_unget"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "sigprocmask_internal"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "sscanf_eof"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "statvfs"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "strverscmp"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "syscall_sign_extend"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "tls_get_new_dtv"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "uselocale_0"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "wcsncpy_read_overflow"},
        {"./runtest.exe", "-w", "entry-dynamic.exe", "wcsstr_false_negative"},
    };
    for (int i = 0; i < 111; i++) {
        pid = exec("runtest.exe", (char *const *)run_dynamic_args[i], NULL);
        res = 0;
        waitpid(pid, &res, 0);
    }
}

static void lmbench_test(bool execute) {
    if (!execute) {
        return;
    }
    printf("run lmbench_testcode.sh\n");
}

static void test_all() {
    time_test(false);
    busybox_test(false);
    lua_test(false);
    iozone_test(false);
    libc_test(false);
    lmbench_test(false);
}

static void test() {
    if (false) {
        test_all();
    }
    // time_test(true);
    // busybox_test(true);
    // lua_test(true);
    libc_test(true);
    // char *args[5] = {"./runtest.exe", "-w", "entry-static.exe", "fscanf"};
    // int pid = exec("runtest.exe", (char *const *)args, NULL);
    // int res = 0;
    // waitpid(pid, &res, 0);
    // printf("\ntest result: %d\n", res >> 8);
}
#endif

int main() {
#ifdef FINAL
    test();
    sys_poweroff();
    return 0;
#else
    for (int i = 0; i < CURRENT_TASK_NUM; i++) {
        int pid = spawn(task_names[i]);
        waitpid(pid, NULL, 0);
    }
#endif
    BEGIN;
    char input_buffer[SHELL_INPUT_MAX_WORDS] = {0};
    char cmd[SHELL_CMD_MAX_LENGTH] = {0};
    char arg[SHELL_ARG_MAX_NUM][SHELL_ARG_MAX_LENGTH] = {0};
    memset(arg, 0, SHELL_ARG_MAX_NUM * SHELL_ARG_MAX_LENGTH);
    char ch;
    int input_length = 0, arg_idx = 0, cmd_res = 0;
    bool cmd_found = false;
    while (1) {
        if (cmd_res == 0) {
            printf("\n(> ^_^ ) > ");
        } else {
            printf("\n(> x_x ) > ");
        }
        // char dirname[SHELL_ARG_MAX_LENGTH];
        // #ifndef DEBUG_WITHOUT_INIT_FS
        // pwd(dirname);
        // #endif
        // printf("%s",dirname);
        // printf(" > ");
        // 3-^C 4-^D, 8-^H(backspace), 9-^I(\t) 10-^J(new line), 13-^M(\r), 24-^X(cancel), 127-Del, 32~126 readable char
        // note: backspace maybe 8('\b') or 127(delete)
        while (input_length < SHELL_INPUT_MAX_WORDS) {
            while ((ch = getchar()) == -1) {}
            if (ch == 3 || ch == 4 || ch == 24) {
                putchar('^');
                goto clear_and_next;
            } else if (ch == 9) {
                // now we assume tab is space
                putchar(32);
                input_buffer[input_length++] = 32;
            } else if (ch == 10 || ch == 13) {
                if (input_length == 0) {
                    goto clear_and_next;
                }
                break;
            } else if (ch > 31 && ch < 127) {
                putchar(ch);
                input_buffer[input_length++] = ch;
            } else if (ch == 8 || ch == 127) {
                if (input_length > 0) {
                    putchar(ch);
                    input_buffer[--input_length] = 0;
                }
            }
        }
        // process with input
        // symbol for calculator and pipe will be done through extra work
        char *parse = input_buffer;
        parse = strtok(cmd, parse, ' ', SHELL_CMD_MAX_LENGTH);
        while ((parse = strtok(arg[arg_idx++], parse, ' ', SHELL_ARG_MAX_LENGTH)) != NULL && arg_idx < SHELL_ARG_MAX_NUM)
            ;

        // check whether the command is valid
        for (int i = 0; i < SUPPORTED_CMD_NUM; i++) {
            if (strcmp(cmd, cmd_table[i].cmd_full_name) == 0 || strcmp(cmd, cmd_table[i].cmd_alias) == 0) {
                cmd_found = true;
                cmd_res = cmd_table[i].handler(arg_idx - 1, (char **)arg);
                break;
            }
        }

        if (!cmd_found) {
            cmd_res = cmd_table[0].handler(arg_idx - 1, (char **)arg);
        }
        if (cmd_res < 0) {
            panic(cmd_error);
        }

        // clear and prepare for next input
    clear_and_next:
        memset(input_buffer, 0, sizeof(input_buffer));
        memset(cmd, 0, sizeof(cmd));
        memset(arg, 0, SHELL_ARG_MAX_NUM * SHELL_ARG_MAX_LENGTH);
        input_length = 0;
        arg_idx = 0;
        cmd_res = 0;
        cmd_found = false;
    }
    return 0;
}
