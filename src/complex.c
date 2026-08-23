/*
 * 复数库（独立动态库）：复数四则运算、指数/对数/开根、模/辐角。
 * 只定义函数并把描述交给主程序注册，不链接核心引擎。
 */

#include "complex.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CalcComplex calc_c_add(CalcComplex a, CalcComplex b) {
    CalcComplex r = { a.re + b.re, a.im + b.im };
    return r;
}

CalcComplex calc_c_sub(CalcComplex a, CalcComplex b) {
    CalcComplex r = { a.re - b.re, a.im - b.im };
    return r;
}

CalcComplex calc_c_mul(CalcComplex a, CalcComplex b) {
    CalcComplex r = { a.re * b.re - a.im * b.im,
                      a.re * b.im + a.im * b.re };
    return r;
}

CalcComplex calc_c_div(CalcComplex a, CalcComplex b) {
    double d = b.re * b.re + b.im * b.im;
    CalcComplex r = { 0, 0 };
    if (d == 0.0) { r.re = NAN; r.im = NAN; return r; }
    r.re = (a.re * b.re + a.im * b.im) / d;
    r.im = (a.im * b.re - a.re * b.im) / d;
    return r;
}

CalcComplex calc_c_exp(CalcComplex a) {
    double m = exp(a.re);
    CalcComplex r = { m * cos(a.im), m * sin(a.im) };
    return r;
}

CalcComplex calc_c_log(CalcComplex a) {
    double mag = calc_c_abs(a);
    CalcComplex r = { log(mag), atan2(a.im, a.re) /* 主值，弧度 */ };
    return r;
}

CalcComplex calc_c_sqrt(CalcComplex a) {
    double mag = calc_c_abs(a);
    double r = sqrt((mag + a.re) / 2.0);
    double i = (a.im >= 0.0 ? 1.0 : -1.0) * sqrt((mag - a.re) / 2.0);
    CalcComplex res = { r, i };
    return res;
}

double calc_c_abs(CalcComplex a) {
    return hypot(a.re, a.im);
}

double calc_c_arg(CalcComplex a, CalcAngleMode mode) {
    double rad = atan2(a.im, a.re);
    switch (mode) {
        case CALC_MODE_DEG:  return rad * 180.0 / M_PI;
        case CALC_MODE_GRAD: return rad * 200.0 / M_PI;
        case CALC_MODE_RAD:
        default:             return rad;
    }
}

CalcComplex calc_c_from_polar(double r, double theta) {
    CalcComplex res = { r * cos(theta), r * sin(theta) };
    return res;
}

/* —— 供表达式引擎注册的复数函数（返回标量）—— */

static double cx_cabs(const double *a, int n, CalcAngleMode mode,
                      char *err, size_t es) {
    (void)mode;
    if (n != 2) { if (es) snprintf(err, es, "cabs 需要 2 个参数：cabs(实部, 虚部)"); return 0.0; }
    CalcComplex z = { a[0], a[1] };
    return calc_c_abs(z);
}

static double cx_carg(const double *a, int n, CalcAngleMode mode,
                      char *err, size_t es) {
    if (n != 2) { if (es) snprintf(err, es, "carg 需要 2 个参数：carg(实部, 虚部)"); return 0.0; }
    CalcComplex z = { a[0], a[1] };
    return calc_c_arg(z, mode);
}

static const calc_function g_cx_fns[] = {
    {"cabs", cx_cabs},
    {"carg", cx_carg},
};

const calc_function *calc_complex_functions(size_t *count) {
    if (count != NULL) *count = sizeof(g_cx_fns) / sizeof(g_cx_fns[0]);
    return g_cx_fns;
}
