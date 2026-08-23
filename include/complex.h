/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

#ifndef COMPLEX_H
#define COMPLEX_H

#include <stddef.h>
#include "calculator.h"   /* calc_function、CalcAngleMode、CalcComplex */

/* —— 复数运算（独立算法库，可直接用于其它工程）—— */
CalcComplex calc_c_add(CalcComplex a, CalcComplex b);
CalcComplex calc_c_sub(CalcComplex a, CalcComplex b);
CalcComplex calc_c_mul(CalcComplex a, CalcComplex b);
CalcComplex calc_c_div(CalcComplex a, CalcComplex b);
CalcComplex calc_c_exp(CalcComplex a);
CalcComplex calc_c_log(CalcComplex a);
CalcComplex calc_c_sqrt(CalcComplex a);
double       calc_c_abs(CalcComplex a);
double       calc_c_arg(CalcComplex a, CalcAngleMode mode);
CalcComplex  calc_c_from_polar(double r, double theta);

/* 复数三角/双曲/反三角/幂 */
CalcComplex calc_c_sin(CalcComplex z);
CalcComplex calc_c_cos(CalcComplex z);
CalcComplex calc_c_tan(CalcComplex z);
CalcComplex calc_c_sinh(CalcComplex z);
CalcComplex calc_c_cosh(CalcComplex z);
CalcComplex calc_c_tanh(CalcComplex z);
CalcComplex calc_c_asin(CalcComplex z);
CalcComplex calc_c_acos(CalcComplex z);
CalcComplex calc_c_atan(CalcComplex z);
CalcComplex calc_c_pow(CalcComplex base, CalcComplex e);

/*
 * 返回本库注册给引擎的复数函数描述（cabs 模、carg 辐角）。
 * 主程序用 calc_register_functions() 注册。
 */
const calc_function *calc_complex_functions(size_t *count);

#endif /* COMPLEX_H */
