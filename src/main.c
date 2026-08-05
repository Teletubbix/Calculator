#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math_add.h"
#include "math_subtract.h"
#include "math_multiply.h"
#include "math_divide.h"
#include "math_power.h"
#include "math_sqrt.h"
#include "print_error.h"
#include "print_output.h"
#include "expr_eval.h"
#include "linenoise.h"

#define APP_VERSION "1.6.0"

int main() {
    int choice;
    double a, b, result;
    char op;

    linenoiseHistorySetMaxLen(100);

    printf("Calculator v%s (C语言学习项目)\n", APP_VERSION);
#ifdef _WIN32
    printf("Platform: Windows\n");
#elif __linux__
    printf("Platform: Linux\n");
#else
    printf("Platform: Unknown\n");
#endif

    while (1) {
        printf("\n========== 科学计算器 ==========\n");
        printf("1. 加法\n2. 减法\n3. 乘法\n4. 除法\n");
        printf("5. 乘方 (a^b)\n6. 开平方根 (√a)\n");
        printf("7. 表达式计算 (支持 + - * /, 括号, sin, cos, tan, sqrt, ln, log, π/pi, e)\n");
        printf("8. 退出\n");
        printf("请选择 (1-8): ");

        if (scanf("%d", &choice) != 1) {
            print_error("输入无效，请输入数字！");
            while (getchar() != '\n');
            continue;
        }

        if (choice == 8) { printf("再见！\n"); break; }
        if (choice < 1 || choice > 7) {
            print_error("无效选项，请输入1-7！");
            continue;
        }

        if (choice >= 1 && choice <= 6) {
            printf("请输入第一个数字 (a): ");
            if (scanf("%lf", &a) != 1) {
                print_error("输入不是有效数字！");
                while (getchar() != '\n');
                continue;
            }
            if (choice != 6) {
                printf("请输入第二个数字 (b): ");
                if (scanf("%lf", &b) != 1) {
                    print_error("输入不是有效数字！");
                    while (getchar() != '\n');
                    continue;
                }
            } else {
                b = 0;
            }

            switch (choice) {
                case 1: result = add(a, b); op = '+'; break;
                case 2: result = subtract(a, b); op = '-'; break;
                case 3: result = multiply(a, b); op = '*'; break;
                case 4:
                    if (b == 0) { print_error("除数不能为0！"); continue; }
                    result = divide(a, b); op = '/'; break;
                case 5:
                    result = power(a, b); op = '^'; break;
                case 6:
                    if (a < 0) { print_error("负数没有实数平方根！"); continue; }
                    result = square_root(a);
                    printf("√%.2f = %.2f\n", a, result);
                    continue;
                default: print_error("未知错误！"); continue;
            }
            print_result(a, op, b, result);
        }

        else if (choice == 7) {
            char *input = linenoise("请输入数学表达式: ");
            if (input == NULL) {
                print_error("读取输入失败！");
                continue;
            }
            if (strlen(input) == 0) {
                linenoiseFree(input);
                continue;
            }
            linenoiseHistoryAdd(input);
            double expr_result = evaluate_expression(input);
            printf("计算结果: %.4f\n", expr_result);
            linenoiseFree(input);
        }
    }

    return 0;
}