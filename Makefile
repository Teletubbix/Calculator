# Calculator Makefile
# 常用命令：
#   make          编译（输出到 bin/Calculator）
#   make run      编译并进入交互模式
#   make test     编译并运行自动测试
#   make clean    删除编译产物

CC      ?= cc
CFLAGS  ?= -O2
CFLAGS  += -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
LDLIBS  += -lm

BIN_DIR := bin
TARGET  := $(BIN_DIR)/Calculator
TEST    := $(BIN_DIR)/calculator_tests

SRC_CORE := src/calculator.c
SRC_MAIN := src/main.c
SRC_TEST := tests/test_calculator.c
HDR      := include/calculator.h

.PHONY: all run test clean

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(TARGET): $(SRC_MAIN) $(SRC_CORE) $(HDR) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_MAIN) $(SRC_CORE) -o $@ $(LDLIBS)

$(TEST): $(SRC_TEST) $(SRC_CORE) $(HDR) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_TEST) $(SRC_CORE) -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

test: $(TEST)
	./$(TEST)

clean:
	rm -rf $(BIN_DIR)
