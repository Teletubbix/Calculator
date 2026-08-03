#include "print_error.h"
#include <stdio.h>

void print_error(const char *msg) {
    // 用换行和 [ERROR] 前缀让信息集中，不加花哨边框
    fprintf(stderr, "\n[ERROR] %s\n\n", msg);
}