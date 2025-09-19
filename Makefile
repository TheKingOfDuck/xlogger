CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGET = xlogger
SOURCE = xlogger.c

# 系统检测
UNAME_S := $(shell uname -s)
DISTRO := $(shell if [ -f /etc/os-release ]; then grep ^ID= /etc/os-release | cut -d= -f2 | tr -d '"'; elif [ -f /etc/redhat-release ]; then echo "rhel"; elif [ -f /etc/debian_version ]; then echo "debian"; else echo "unknown"; fi)

# 设置链接库
ifeq ($(UNAME_S),Linux)
    LDFLAGS = -lutil
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -lutil
endif
ifeq ($(UNAME_S),FreeBSD)
    LDFLAGS = -lutil
endif
ifeq ($(UNAME_S),OpenBSD)
    LDFLAGS = -lutil
endif

.PHONY: all clean install setup deps check-deps help

all: check-deps $(TARGET)

# 检查依赖是否已安装
check-deps:
ifeq ($(UNAME_S),Linux)
	@echo "检测Linux系统，检查依赖..."
	@if ! echo '#include <pty.h>' | $(CC) -E - >/dev/null 2>&1; then \
		echo "缺少pty.h头文件，尝试安装依赖..."; \
		$(MAKE) deps; \
	else \
		echo "依赖检查通过"; \
	fi
endif

# 自动安装依赖
deps:
ifeq ($(UNAME_S),Linux)
	@echo "检测到Linux系统，安装libutil开发包..."
ifeq ($(DISTRO),ubuntu)
	@echo "检测到Ubuntu系统"
	sudo apt-get update && sudo apt-get install -y libutil-dev
else ifeq ($(DISTRO),debian)
	@echo "检测到Debian系统"
	sudo apt-get update && sudo apt-get install -y libutil-dev
else ifeq ($(DISTRO),fedora)
	@echo "检测到Fedora系统"
	sudo dnf install -y libutempter-devel
else ifeq ($(DISTRO),rhel)
	@echo "检测到RHEL/CentOS系统"
	sudo yum install -y libutempter-devel || sudo dnf install -y libutempter-devel
else ifeq ($(DISTRO),centos)
	@echo "检测到CentOS系统"
	sudo yum install -y libutempter-devel || sudo dnf install -y libutempter-devel
else ifeq ($(DISTRO),opensuse)
	@echo "检测到openSUSE系统"
	sudo zypper install -y libutil-devel
else ifeq ($(DISTRO),arch)
	@echo "检测到Arch Linux系统"
	sudo pacman -S --noconfirm util-linux
else
	@echo "未识别的Linux发行版: $(DISTRO)"
	@echo "请手动安装pty开发库:"
	@echo "  Ubuntu/Debian: sudo apt-get install libutil-dev"
	@echo "  Fedora/RHEL:   sudo dnf install libutempter-devel"
	@echo "  Arch Linux:    sudo pacman -S util-linux"
	@echo "  openSUSE:      sudo zypper install libutil-devel"
	@exit 1
endif
else
	@echo "$(UNAME_S)系统无需安装额外依赖"
endif

# 手动安装依赖（提示用户）
setup:
	@echo "系统信息:"
	@echo "  操作系统: $(UNAME_S)"
ifeq ($(UNAME_S),Linux)
	@echo "  发行版: $(DISTRO)"
endif
	@echo ""
	@echo "运行 'make deps' 自动安装依赖，或手动安装:"
ifeq ($(UNAME_S),Linux)
ifeq ($(DISTRO),ubuntu)
	@echo "  sudo apt-get install libutil-dev"
else ifeq ($(DISTRO),debian)
	@echo "  sudo apt-get install libutil-dev"
else ifeq ($(DISTRO),fedora)
	@echo "  sudo dnf install libutempter-devel"
else ifeq ($(DISTRO),rhel)
	@echo "  sudo yum install libutempter-devel"
else ifeq ($(DISTRO),centos)
	@echo "  sudo yum install libutempter-devel"
else
	@echo "  请参考README.md中的依赖安装说明"
endif
else
	@echo "  macOS系统无需额外依赖"
endif

$(TARGET): $(SOURCE)
	@echo "编译 $(TARGET)..."
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
	@echo "编译完成!"

clean:
	rm -f $(TARGET) xlogger.log

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

help:
	@echo "xlogger 编译选项:"
	@echo ""
	@echo "  make           - 自动检测依赖并编译 (推荐)"
	@echo "  make all       - 同上"
	@echo "  make setup     - 显示系统信息和依赖安装命令"
	@echo "  make deps      - 自动安装依赖 (需要sudo权限)"
	@echo "  make clean     - 清理编译文件"
	@echo "  make install   - 安装到 /usr/local/bin/"
	@echo "  make help      - 显示此帮助信息"
	@echo ""
	@echo "支持的系统: Linux (Ubuntu/Debian/Fedora/RHEL/CentOS/Arch/openSUSE), macOS"

.DEFAULT_GOAL := all