# Calculator Makefile
#   make            编译（各算法编译成独立 .so，并链接出 bin/Calculator）
#   make run        编译并进入交互模式
#   make test       编译并运行自动测试
#   make clean      删除编译产物
#
# 分布式架构：每个算法（核心/单位/矩阵/复数）都是独立共享库，
# bug 只改对应源文件重新编译该库即可，互不影响。

CC      ?= cc
CFLAGS  ?= -O2
CFLAGS  += -std=c11 -Wall -Wextra -Wpedantic
CPPFLAGS += -Iinclude
LDLIBS  += -lm

BIN_DIR := bin
TARGET  := $(BIN_DIR)/Calculator
TEST    := $(BIN_DIR)/calculator_tests

SRC_MAIN := src/main.c
SRC_TEST := tests/test_calculator.c
HDR      := include/calculator.h include/units.h include/matrix.h include/complex.h

# 各算法库（独立共享库）
CORE_LIB    := $(BIN_DIR)/libcalc_core.so
UNITS_LIB   := $(BIN_DIR)/libcalc_units.so
MATRIX_LIB  := $(BIN_DIR)/libcalc_matrix.so
COMPLEX_LIB := $(BIN_DIR)/libcalc_complex.so
ALG_LIBS    := $(CORE_LIB) $(UNITS_LIB) $(MATRIX_LIB) $(COMPLEX_LIB)

.PHONY: all run test clean

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(CORE_LIB): src/calculator.c include/calculator.h | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/calculator.c -o $@ $(LDLIBS)

$(UNITS_LIB): src/units.c include/units.h | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/units.c -o $@ $(LDLIBS)

$(MATRIX_LIB): src/matrix.c include/matrix.h include/calculator.h | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/matrix.c -o $@

$(COMPLEX_LIB): src/complex.c include/complex.h include/calculator.h | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared src/complex.c -o $@ $(LDLIBS)

$(TARGET): $(SRC_MAIN) $(ALG_LIBS) $(HDR) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_MAIN) -o $@ \
		-L$(BIN_DIR) -lcalc_core -lcalc_units -lcalc_matrix -lcalc_complex \
		-Wl,-rpath,'$$ORIGIN' $(LDLIBS)

# 测试直接编译源码（含矩阵/复数），便于单测
$(TEST): $(SRC_TEST) src/calculator.c src/matrix.c src/complex.c $(HDR) | $(BIN_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC_TEST) src/calculator.c src/matrix.c src/complex.c -o $@ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

test: $(TEST)
	./$(TEST)

clean:
	rm -rf $(BIN_DIR)
