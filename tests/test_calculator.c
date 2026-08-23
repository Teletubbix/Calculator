/*
 * Calculator 自动测试
 *
 * 覆盖需求中的全部基本运算：
 * 加、减、乘、除、乘方、开根、对数、阶乘、sin、cos、tan，
 * 以及常量 pi / π / e 和角度制三角函数。
 */

#include "calculator.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

static int failures = 0;

static int near(double actual, double expected, double tolerance) {
    return fabs(actual - expected) <= tolerance;
}

static void expect_value(const char *expr, double expected, double tolerance) {
    double result = 0.0;
    char error[512] = {0};
    int rc = calc_evaluate(expr, &result, error, sizeof(error));
    if (rc != 0 || !near(result, expected, tolerance)) {
        printf("FAIL  %-24s 期望=%g  实际=%g  错误=%s\n",
               expr, expected, result, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  %-24s = %g\n", expr, result);
    }
}

static void expect_value_ans(const char *expr, double ans, double expected, double tolerance) {
    double result = 0.0;
    char error[512] = {0};
    int rc = calc_evaluate_with_ans(expr, ans, 1, &result, error, sizeof(error));
    if (rc != 0 || !near(result, expected, tolerance)) {
        printf("FAIL  %-24s 期望=%g  实际=%g  错误=%s\n",
               expr, expected, result, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  %-24s = %g\n", expr, result);
    }
}

static void expect_error(const char *expr) {
    double result = 0.0;
    char error[512] = {0};
    int rc = calc_evaluate(expr, &result, error, sizeof(error));
    if (rc == 0) {
        printf("FAIL  %-24s 本应报错，却得到 %g\n", expr, result);
        failures++;
    } else {
        printf("PASS  %-24s 正确报错：%s", expr, error);
    }
}

static void expect_value_mode(const char *expr, CalcAngleMode mode, double expected, double tolerance) {
    double result = 0.0;
    char error[512] = {0};
    int rc = calc_evaluate_mode(expr, mode, 0, 0, &result, error, sizeof(error));
    if (rc != 0 || !near(result, expected, tolerance)) {
        printf("FAIL  %-24s(模式=%d) 期望=%g  实际=%g  错误=%s\n",
               expr, (int)mode, expected, result, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  %-24s(模式=%d) = %g\n", expr, (int)mode, result);
    }
}

int main(void) {
    /* 加减乘除 */
    expect_value("2+3", 5, 1e-12);
    expect_value("10-4", 6, 1e-12);
    expect_value("3*4", 12, 1e-12);
    expect_value("7/2", 3.5, 1e-12);
    expect_value("2+3*4", 14, 1e-12);
    expect_value("(2+3)*4", 20, 1e-12);

    /* 乘方和开根 */
    expect_value("2^10", 1024, 1e-9);
    expect_value("2^3^2", 512, 1e-9);
    expect_value("2^-2", 0.25, 1e-12);
    expect_value("pow(3,4)", 81, 1e-9);
    expect_value("sqrt(9)", 3, 1e-12);
    expect_value("sqrt(2)*sqrt(2)", 2, 1e-12);

    /* 对数 */
    expect_value("log(1000)", 3, 1e-12);
    expect_value("ln(e)", 1, 1e-12);
    expect_value("log2(8)", 3, 1e-12);

    /* 阶乘 */
    expect_value("5!", 120, 1e-9);
    expect_value("0!", 1, 1e-12);
    expect_value("2*3!", 12, 1e-9);

    /* 三角函数（弧度制） */
    expect_value("sin(pi/2)", 1, 1e-12);
    expect_value("cos(0)", 1, 1e-12);
    expect_value("tan(pi/4)", 1, 1e-9);

    /* 三角函数（角度制） */
    expect_value("sind(30)", 0.5, 1e-12);
    expect_value("cosd(60)", 0.5, 1e-12);
    expect_value("tand(45)", 1, 1e-9);

    /* 常量 pi / π / e */
    expect_value("2*pi", 2 * M_PI, 1e-12);
    expect_value("2*π", 2 * M_PI, 1e-12);
    expect_value("e^1", M_E, 1e-12);

    /* Ans 记忆功能 */
    expect_value_ans("e^2", 0.0, M_E * M_E, 1e-12);
    expect_value_ans("sin(Ans)", M_E * M_E, sin(M_E * M_E), 1e-12);
    expect_value_ans("Ans+1", 41.0, 42.0, 1e-12);
    expect_error("ans");

    /* 一元正负号与优先级 */
    expect_value("-3^2", -9, 1e-12);
    expect_value("(-3)^2", 9, 1e-12);
    expect_value("--5", 5, 1e-12);
    expect_value("10/2/5", 1, 1e-12);

    /* 错误输入 */
    expect_error("1/0");
    expect_error("sqrt(-1)");
    expect_error("ln(0)");
    expect_error("log(-10)");
    expect_error("1.5!");
    expect_error("(-2)^0.5");
    expect_error("(2+3");
    expect_value("2++3", 5, 1e-12);
    expect_value("2+-3", -1, 1e-12);
    expect_error("2pi");
    expect_error("foo(1)");
    expect_error("1 2");
    expect_error("");

    /* 幂与负底数、特殊值 */
    expect_value("pow(2,-2)", 0.25, 1e-12);
    expect_value("(-2)^3", -8, 1e-12);
    expect_value("0^0", 1, 1e-12);
    expect_value("sqrt(2)^2", 2, 1e-12);
    expect_value("cos(2*pi)", 1, 1e-9);

    /* 溢出与定义域边界 */
    expect_error("1e999");
    expect_error("1e308*1e308");
    expect_error("2^1024");
    expect_error("exp(1000)");
    expect_error("1+");
    expect_error("2(3)");
    expect_error("   ");
    expect_error("7/0.0");

    /* —— v4.0 新增一元函数 —— */
    expect_value("asin(1)", M_PI / 2, 1e-12);
    expect_value("acos(0)", M_PI / 2, 1e-12);
    expect_value("atan(1)", M_PI / 4, 1e-12);
    expect_value("asind(1)", 90, 1e-12);
    expect_value("acosd(0)", 90, 1e-12);
    expect_value("atand(1)", 45, 1e-12);
    expect_value("sinh(0)", 0, 1e-12);
    expect_value("cosh(0)", 1, 1e-12);
    expect_value("tanh(0)", 0, 1e-12);
    expect_value("floor(3.7)", 3, 1e-12);
    expect_value("ceil(3.2)", 4, 1e-12);
    expect_value("round(3.5)", 4, 1e-12);
    expect_value("trunc(-3.7)", -3, 1e-12);
    expect_value("sign(-5)", -1, 1e-12);
    expect_value("sign(5)", 1, 1e-12);

    /* —— v4.0 新增二元函数 —— */
    expect_value("atan2(0,1)", 0, 1e-12);
    expect_value("mod(10,3)", 1, 1e-12);
    expect_value("gcd(12,18)", 6, 1e-12);
    expect_value("lcm(4,6)", 12, 1e-12);
    expect_value("comb(10,3)", 120, 1e-12);
    expect_value("perm(10,3)", 720, 1e-12);
    expect_value("logn(8,2)", 3, 1e-12);

    /* —— v4.0 新增常量 —— */
    expect_value("tau", 2 * M_PI, 1e-12);
    expect_value("phi", (1 + sqrt(5)) / 2, 1e-12);

    /* —— v4.0 角度模式 —— */
    expect_value_mode("sin(30)", CALC_MODE_DEG, 0.5, 1e-12);
    expect_value_mode("sin(pi/2)", CALC_MODE_RAD, 1, 1e-12);
    expect_value_mode("sin(100)", CALC_MODE_GRAD, 1, 1e-12);
    expect_value_mode("atan(1)", CALC_MODE_DEG, 45, 1e-9);
    expect_value_mode("cos(100)", CALC_MODE_GRAD, 0, 1e-9);

    /* —— v4.0 新增函数的错误用例 —— */
    expect_error("asin(2)");
    expect_error("acos(-2)");
    expect_value("gcd(12,0)", 12, 1e-12);
    expect_error("comb(3,5)");
    expect_error("logn(8,1)");
    expect_error("mod(5,0)");

    /* —— v4.1 矩阵函数（方阵行列式/迹）—— */
    expect_value("det2(1,2,3,4)", -2, 1e-12);
    expect_value("trace2(1,2,3,4)", 5, 1e-12);
    expect_value("det3(1,2,3,4,5,6,7,8,9)", 0, 1e-12);
    expect_value("det3(2,0,0,0,3,0,0,0,4)", 24, 1e-12);
    expect_value("trace3(1,2,3,4,5,6,7,8,9)", 15, 1e-12);
    expect_error("det2(1,2,3)");
    expect_error("det3(1,2,3)");

    printf("\n测试结束：%s\n", failures == 0 ? "全部通过" : "存在失败用例");
    return failures == 0 ? 0 : 1;
}
