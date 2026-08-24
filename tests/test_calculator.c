/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

/*
 * Calculator 自动测试
 *
 * 覆盖需求中的全部基本运算：
 * 加、减、乘、除、乘方、开根、对数、阶乘、sin、cos、tan，
 * 以及常量 pi / π / e 和角度制三角函数。
 */

#include "calculator.h"
#include "matrix.h"
#include "complex.h"
#include "units.h"
#include "db.h"

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

static int near(double actual, double expected, double tolerance);

/* 单位换算测试 */
static void expect_unit(double value, const char *from, const char *to, double expected, double tol) {
    double out = 0.0; char err[256] = {0};
    if (unit_convert(value, from, to, &out, err, sizeof(err)) != 0 || !near(out, expected, tol)) {
        printf("FAIL  convert %.9g %s -> %s  期望 %g  实际 %g  错误 %s\n",
               value, from, to, expected, out, err);
        failures++;
    } else {
        printf("PASS  convert %.9g %s -> %s = %g\n", value, from, to, out);
    }
}

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

static void expect_complex(const char *expr, double re, double im, double tolerance) {
    CalcComplex result = {0, 0};
    char error[512] = {0};
    int rc = calc_evaluate_complex(expr, CALC_MODE_RAD, (CalcComplex){0,0}, 0,
                                   &result, error, sizeof(error));
    if (rc != 0 || !near(result.re, re, tolerance) || !near(result.im, im, tolerance)) {
        printf("FAIL  %-24s 期望=%g%+gi  实际=%g%+gi  错误=%s\n",
               expr, re, im, result.re, result.im, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  %-24s = %g%+gi\n", expr, result.re, result.im);
    }
}

static void expect_complex_mode(const char *expr, CalcAngleMode mode, double re, double im, double tolerance) {
    CalcComplex result = {0, 0};
    char error[512] = {0};
    int rc = calc_evaluate_complex(expr, mode, (CalcComplex){0,0}, 0,
                                   &result, error, sizeof(error));
    if (rc != 0 || !near(result.re, re, tolerance) || !near(result.im, im, tolerance)) {
        printf("FAIL  %-24s(模式=%d) 期望=%g%+gi  实际=%g%+gi  错误=%s\n",
               expr, (int)mode, re, im, result.re, result.im, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  %-24s(模式=%d) = %g%+gi\n", expr, (int)mode, result.re, result.im);
    }
}

/* —— v6.0 数值工具测试辅助 —— */
static void expect_eval_x(const char *expr, double x, double re, double im, double tolerance) {
    CalcComplex result = {0, 0};
    char error[512] = {0};
    int rc = calc_evaluate_x(expr, CALC_MODE_RAD, x, &result, error, sizeof(error));
    if (rc != 0 || !near(result.re, re, tolerance) || !near(result.im, im, tolerance)) {
        printf("FAIL  f(%g):%s 期望=%g%+gi  实际=%g%+gi  错误=%s\n",
               x, expr, re, im, result.re, result.im, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  f(%g):%s = %g%+gi\n", x, expr, result.re, result.im);
    }
}

static void expect_integ(const char *expr, double a, double b, double re, double im, double tolerance) {
    CalcComplex result = {0, 0};
    char error[512] = {0};
    int rc = calc_ninteg(expr, CALC_MODE_RAD, a, b, &result, error, sizeof(error));
    if (rc != 0 || !near(result.re, re, tolerance) || !near(result.im, im, tolerance)) {
        printf("FAIL  ∫[%g,%g]%s 期望=%g%+gi  实际=%g%+gi  错误=%s\n",
               a, b, expr, re, im, result.re, result.im, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  ∫[%g,%g]%s = %g%+gi\n", a, b, expr, result.re, result.im);
    }
}

static void expect_deriv(const char *expr, double x0, double re, double tolerance) {
    CalcComplex result = {0, 0};
    char error[512] = {0};
    int rc = calc_nderiv(expr, CALC_MODE_RAD, x0, &result, error, sizeof(error));
    if (rc != 0 || !near(result.re, re, tolerance) || !near(result.im, 0, tolerance)) {
        printf("FAIL  f'(%g):%s 期望=%g  实际=%g  错误=%s\n",
               x0, expr, re, result.re, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  f'(%g):%s = %g\n", x0, expr, result.re);
    }
}

static void expect_root(const char *expr, double a, double b, double expected, double tolerance) {
    double result = 0.0; char error[512] = {0};
    int rc = calc_root(expr, CALC_MODE_RAD, a, b, &result, error, sizeof(error));
    if (rc != 0 || !near(result, expected, tolerance)) {
        printf("FAIL  root[%g,%g]:%s 期望=%g  实际=%g  错误=%s\n",
               a, b, expr, expected, result, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  root[%g,%g]:%s = %g\n", a, b, expr, result);
    }
}

static void expect_sum(const char *expr, long a, long b, double expected, double tolerance) {
    CalcComplex result = {0, 0}; char error[512] = {0};
    int rc = calc_sum(expr, CALC_MODE_RAD, a, b, &result, error, sizeof(error));
    if (rc != 0 || !near(result.re, expected, tolerance) || !near(result.im, 0, tolerance)) {
        printf("FAIL  Σ[%ld..%ld]:%s 期望=%g  实际=%g  错误=%s\n",
               a, b, expr, expected, result.re, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  Σ[%ld..%ld]:%s = %g\n", a, b, expr, result.re);
    }
}

static void expect_prod(const char *expr, long a, long b, double expected, double tolerance) {
    CalcComplex result = {0, 0}; char error[512] = {0};
    int rc = calc_prod(expr, CALC_MODE_RAD, a, b, &result, error, sizeof(error));
    if (rc != 0 || !near(result.re, expected, tolerance) || !near(result.im, 0, tolerance)) {
        printf("FAIL  Π[%ld..%ld]:%s 期望=%g  实际=%g  错误=%s\n",
               a, b, expr, expected, result.re, rc != 0 ? error : "数值不符");
        failures++;
    } else {
        printf("PASS  Π[%ld..%ld]:%s = %g\n", a, b, expr, result.re);
    }
}

int main(void) {
    /* 注册算法库（矩阵/复数），这样 det/det/cabs/carg 才能在表达式里生效 */
    size_t n;
    const calc_function *f = calc_matrix_functions(&n);
    calc_register_functions(f, n);
    f = calc_complex_functions(&n);
    calc_register_functions(f, n);
    f = calc_db_functions(&n);
    calc_register_functions(f, n);

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
    expect_complex("sqrt(-1)", 0, 1, 1e-9);          /* sqrt(-1) = i */
    expect_error("ln(0)");
    expect_complex("log(-10)", 1, M_PI / log(10), 1e-9);
    expect_error("1.5!");
    expect_complex("(-2)^0.5", 0, sqrt(2), 1e-9);
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
    expect_value("2^10", 1024, 1e-9);
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
    expect_complex("asin(2)", M_PI / 2, -log(2.0 + sqrt(3.0)), 1e-9);
    expect_complex("acos(-2)", M_PI, -log(2.0 + sqrt(3.0)), 1e-9);
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

    /* —— v4.2 复数（模/辐角，通信相量/阻抗）—— */
    expect_value("cabs(3,4)", 5, 1e-12);
    expect_value("cabs(-3,-4)", 5, 1e-12);
    expect_value("carg(0,1)", M_PI / 2, 1e-12);
    expect_value("carg(1,0)", 0, 1e-12);
    expect_value("carg(-1,0)", M_PI, 1e-12);
    expect_value("cabs(0,0)", 0, 1e-12);
    expect_value_mode("carg(0,1)", CALC_MODE_DEG, 90, 1e-12);
    expect_value_mode("carg(0,1)", CALC_MODE_GRAD, 100, 1e-12);
    expect_error("cabs(3)");
    expect_error("carg(1,2,3)");

    /* —— v4.3 完整复数（字面量与四则/复数函数）—— */
    expect_complex("3+4j", 3, 4, 1e-12);
    expect_complex("4j", 0, 4, 1e-12);
    expect_complex("2i", 0, 2, 1e-12);
    expect_complex("(1+2j)*(3-4j)", 11, 2, 1e-12);
    expect_complex("(3+4j)/(1-2j)", -1, 2, 1e-12);
    expect_complex("sqrt(-1)", 0, 1, 1e-9);
    expect_complex("exp(1j*pi)", -1, 0, 1e-9);
    expect_complex("(1+1j)^2", 0, 2, 1e-9);
    expect_complex("2^0.5", sqrt(2), 0, 1e-9);
    expect_complex("(1+2j)+3", 4, 2, 1e-12);
    expect_complex("1+2j+3i", 1, 5, 1e-12);   /* 1 + 2i + 3i = 1 + 5i */

    /* —— v4.3 复数边界/深入 —— */
    expect_complex("j", 0, 1, 1e-12);
    expect_complex("i", 0, 1, 1e-12);
    expect_complex("3j*2j", -6, 0, 1e-12);          /* 3i·2i = 6i² = -6 */
    expect_complex("1/j", 0, -1, 1e-12);            /* 1/i = -i */
    expect_complex("abs(3+4j)", 5, 0, 1e-12);       /* 模 */
    expect_complex("0^0", 1, 0, 1e-12);             /* 约定 0^0 = 1 */
    expect_complex("sin(1j)", 0, sinh(1), 1e-6);    /* sin(i) = i·sinh(1) */
    expect_complex("(1+2j)^2", -3, 4, 1e-12);       /* (1+2i)² = 1+4i-4 = -3+4i */
    expect_complex("exp(2j)", cos(2), sin(2), 1e-9);/* e^{i·2} */
    expect_complex("sqrt(3+4j)", 2, 1, 1e-9);       /* sqrt(3+4i) = 2+i */
    expect_error("(1+1j)/0");
    expect_error("floor(1+2j)");

    /* —— v4.6 极坐标 / ∠ 相量 —— */
    expect_complex("3∠(pi/6)", 3*cos(M_PI/6), 3*sin(M_PI/6), 1e-9);
    expect_complex("3@(pi/6)", 3*cos(M_PI/6), 3*sin(M_PI/6), 1e-9);
    expect_complex_mode("3∠30", CALC_MODE_DEG, 3*cos(30*M_PI/180), 3*sin(30*M_PI/180), 1e-9);
    expect_complex_mode("1∠90", CALC_MODE_DEG, 0, 1, 1e-9);
    expect_complex("5∠(pi/2)*2", 0, 10, 1e-9);
    expect_complex("abs(3∠(pi/3))", 3, 0, 1e-9);

    /* —— 单位换算 —— */
    expect_unit(1, "km", "m", 1000, 1e-9);
    expect_unit(1, "GB", "MB", 1000, 1e-9);
    expect_unit(0, "degC", "degF", 32, 1e-9);
    expect_unit(100, "degC", "degF", 212, 1e-9);
    expect_unit(0, "dBm", "mW", 1, 1e-9);
    expect_unit(10, "dBm", "mW", 10, 1e-9);
    expect_unit(3, "GHz", "MHz", 3000, 1e-9);
    expect_unit(1, "kW", "W", 1000, 1e-9);
    expect_unit(2, "h", "min", 120, 1e-9);
    expect_unit(1, "lb", "kg", 0.45359237, 1e-9);
    expect_unit(30, "degC", "degC", 30, 1e-12);

    /* —— v4.4 dB / 功率级函数 —— */
    expect_value("dbm(100)", 20, 1e-9);        /* 100 mW = 20 dBm */
    expect_value("mw(20)", 100, 1e-9);         /* 20 dBm = 100 mW */
    expect_value("dbw(1)", 0, 1e-9);           /* 1 W = 0 dBW */
    expect_value("w(0)", 1, 1e-9);             /* 0 dBW = 1 W */
    expect_value("pow2db(100)", 20, 1e-9);     /* 100x 功率比 = 20 dB */
    expect_value("db2pow(20)", 100, 1e-9);     /* 20 dB = 100x */
    expect_value("mw(0)", 1, 1e-9);            /* 0 dBm = 1 mW */
    expect_error("pow2db(0)");
    expect_error("dbm(-5)");

    /* —— v6.0 数值工具：x 变量代入 —— */
    expect_eval_x("x^2", 3, 9, 0, 1e-9);
    expect_eval_x("x^2+1", -2, 5, 0, 1e-9);
    expect_eval_x("sin(x)", 0, 0, 0, 1e-9);

    /* —— v6.0 数值积分 —— */
    expect_integ("x^2", 0, 1, 1.0/3.0, 0, 1e-8);
    expect_integ("x^2", 0, 3, 9, 0, 1e-7);
    expect_integ("sin(x)", 0, M_PI, 2, 0, 1e-8);
    expect_integ("exp(x)", 0, 1, M_E - 1, 0, 1e-7);
    expect_integ("2*x+1", 0, 2, 6, 0, 1e-8);      /* x^2 + x 在 [0,2] = 4+2 = 6 */

    /* —— v6.0 数值求导 —— */
    expect_deriv("x^2", 3, 6, 1e-5);
    expect_deriv("sin(x)", 0, 1, 1e-5);
    expect_deriv("x^3", 2, 12, 1e-5);
    expect_deriv("exp(x)", 1, M_E, 1e-5);

    /* —— v6.0 求根（二分法）—— */
    expect_root("x^2-4", 0, 5, 2, 1e-9);
    expect_root("x-1", 0, 3, 1, 1e-9);
    expect_root("sin(x)", 3, 4, M_PI, 1e-7);

    /* —— v6.0 求和 / 连乘 —— */
    expect_sum("x", 1, 5, 15, 1e-9);            /* 1+2+3+4+5 */
    expect_sum("x^2", 1, 5, 55, 1e-9);          /* 1+4+9+16+25 */
    expect_prod("x", 1, 5, 120, 1e-9);          /* 1*2*3*4*5 */
    expect_prod("x", 1, 4, 24, 1e-9);

    printf("\n测试结束：%s\n", failures == 0 ? "全部通过" : "存在失败用例");
    return failures == 0 ? 0 : 1;
}
