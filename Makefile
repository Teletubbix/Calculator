# Calculator Makefile（windows 分支）
#
# 在 Windows 的 MSYS2/MinGW 或 WSL 中可用。
#   make             构建当前平台可用的目标
#   make windows     使用 MinGW 交叉编译 Windows GUI/CLI/测试
#   make run-cli     运行命令行版本（Linux 下验证核心）
#   make test        构建并运行自动测试（Linux 下）
#   make clean       清理编译产物

CC        ?= cc
MINGW_CC  ?= x86_64-w64-mingw32-gcc
CFLAGS    ?= -O2
CFLAGS    += -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS  += -Iinclude
LDLIBS    += -lm

BIN_DIR := bin
CLI     := $(BIN_DIR)/Calculator-cli
TEST    := $(BIN_DIR)/calculator_tests
WIN_GUI := $(BIN_DIR)/Calculator.exe
WIN_CLI := $(BIN_DIR)/Calculator-cli.exe
WIN_TEST:= $(BIN_DIR)/calculator_tests.exe

SRC_CORE := src/calculator.c
SRC_MAIN := src/main.c
SRC_GUI  := src/gui_windows.c
SRC_TEST := tests/test_calculator.c
HDR      := include/calculator.h

.PHONY: all windows test run-cli clean

all: $(CLI) $(TEST)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(CLI): $(SRC_MAIN) $(SRC_CORE) $(HDR) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_MAIN) $(SRC_CORE) -o $@ $(LDLIBS)

$(TEST): $(SRC_TEST) $(SRC_CORE) $(HDR) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_TEST) $(SRC_CORE) -o $@ $(LDLIBS)

windows: $(WIN_GUI) $(WIN_CLI) $(WIN_TEST)

$(WIN_GUI): $(SRC_GUI) $(SRC_CORE) $(HDR) | $(BIN_DIR)
	$(MINGW_CC) $(CPPFLAGS) $(CFLAGS) -municode -mwindows $(SRC_GUI) $(SRC_CORE) -o $@ -lm

$(WIN_CLI): $(SRC_MAIN) $(SRC_CORE) $(HDR) | $(BIN_DIR)
	$(MINGW_CC) $(CPPFLAGS) $(CFLAGS) $(SRC_MAIN) $(SRC_CORE) -o $@ -lm

$(WIN_TEST): $(SRC_TEST) $(SRC_CORE) $(HDR) | $(BIN_DIR)
	$(MINGW_CC) $(CPPFLAGS) $(CFLAGS) $(SRC_TEST) $(SRC_CORE) -o $@ -lm

test: $(TEST)
	./$(TEST)

run-cli: $(CLI)
	./$(CLI)

clean:
	rm -rf $(BIN_DIR)
