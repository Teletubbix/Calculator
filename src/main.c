#include <stdio.h>
#include <stdlib.h>
#include "math_add.h"
#include "math_subtract.h"
#include "math_multiply.h"
#include "math_divide.h"
#include "print_error.h"   // 新增

int main() {
    int choice;
    double a, b, result;
    while (1) {
        printf("\n========== 四则运算计算器 ==========\n");
        printf("1. 加法\n2. 减法\n3. 乘法\n4. 除法\n5. 退出\n");
        printf("请选择 (1-5): ");
        if (scanf("%d", &choice) != 1) {
            print_error("输入无效，请输入数字！");
            while (getchar() != '\n');
            continue;
        }
        if (choice == 5) { printf("再见！\n"); break; }
        if (choice < 1 || choice > 5) {
            print_error("无效选项，请输入1-5！");
            continue;
        }
        printf("请输入第一个数字: ");
        if (scanf("%lf", &a) != 1) {
            print_error("输入不是有效数字！");
            while (getchar() != '\n');
            continue;
        }
        printf("请输入第二个数字: ");
        if (scanf("%lf", &b) != 1) {
            print_error("输入不是有效数字！");
            while (getchar() != '\n');
            continue;
        }
        switch (choice) {
            case 1: result = add(a, b); break;
            case 2: result = subtract(a, b); break;
            case 3: result = multiply(a, b); break;
            case 4:
                if (b == 0) {
                    print_error("除数不能为0！");  // 现在有醒目的框了
                    continue;
                }
                result = divide(a, b);
                break;
        }
        printf("结果: %.2f\n", result);
    }
    return 0;
}