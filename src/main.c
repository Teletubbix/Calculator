#include <stdio.h>
#include <stdlib.h>
#include "math_add.h"
#include "math_subtract.h"
#include "math_multiply.h"
#include "math_divide.h"
#include "math_power.h"
#include "math_sqrt.h"
#include "print_error.h"
#include "print_output.h"

#define APP_VERSION "1.1.0"

int main() {
    int choice;
    double a, b, result;
    char op;

    printf("Calculator v%s (C语言学习项目)\n", APP_VERSION);

    while (1) {
        printf("\n========== 科学计算器 ==========\n");
        printf("1. 加法\n2. 减法\n3. 乘法\n4. 除法\n");
        printf("5. 乘方 (a^b)\n6. 开平方根 (√a)\n7. 退出\n");
        printf("请选择 (1-7): ");

        if (scanf("%d", &choice) != 1) {
            print_error("输入无效，请输入数字！");
            while (getchar() != '\n');
            continue;
        }
        if (choice == 7) { printf("再见！\n"); break; }
        if (choice < 1 || choice > 7) {
            print_error("无效选项，请输入1-7！");
            continue;
        }

        // 获取第一个数字
        printf("请输入第一个数字 (a): ");
        if (scanf("%lf", &a) != 1) {
            print_error("输入不是有效数字！");
            while (getchar() != '\n');
            continue;
        }

        // 对于开平方根，不需要第二个数字；对于其他运算需要
        if (choice != 6) {
            printf("请输入第二个数字 (b): ");
            if (scanf("%lf", &b) != 1) {
                print_error("输入不是有效数字！");
                while (getchar() != '\n');
                continue;
            }
        } else {
            b = 0; // 占位，防止未初始化
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
                if (a < 0) {
                    print_error("负数没有实数平方根！");
                    continue;
                }
                result = square_root(a);
                // 开根是单目运算，直接打印
                printf("√%.2f = %.2f\n", a, result);
                continue; // 跳过下方的双目打印
            default: print_error("未知错误！"); continue;
        }

        // 打印双目运算结果
        print_result(a, op, b, result);
    }
    return 0;
}