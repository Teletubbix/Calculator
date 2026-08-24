/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stddef.h>

/* 复数类型（供复数求值 API 使用；complex.h 也复用此定义） */
typedef struct { double re; double im; } CalcComplex;

#ifdef __cplusplus
extern "C" {
#endif

/* 三角函数角度制 */
typedef enum {
    CALC_MODE_DEG  = 0,  /* 角度制（degree）*/
    CALC_MODE_RAD  = 1,  /* 弧度制（radian，默认）*/
    CALC_MODE_GRAD = 2   /* 百分度（gradian，一圈=400）*/
} CalcAngleMode;

/*
 * 计算一个数学表达式的值（不含 Ans，旧版兼容接口，角度制为弧度）。
 *
 * 参数：
 *   expression        以 NUL 结尾的表达式字符串，例如 "2+3*4"、"sin(pi/2)"。
 *   result            输出参数；计算成功时保存结果。
 *   error_buffer      输出参数；计算失败时保存错误信息（可传 NULL）。
 *   error_buffer_size error_buffer 的可用字节数（error_buffer 为 NULL 时忽略）。
 *
 * 返回值：
 *   0   计算成功。
 *   -1  计算失败（语法错误、定义域错误等），错误原因写入 error_buffer。
 */
int calc_evaluate(const char *expression,
                  double *result,
                  char *error_buffer,
                  size_t error_buffer_size);

/*
 * 计算一个数学表达式的值，并允许表达式使用 Ans 引用上一次计算结果。
 *
 * 参数：
 *   expression        表达式字符串。其中 "ans" 会被替换为 ans 参数的值。
 *   ans               上一次计算结果（仅在 has_ans 非 0 时生效）。
 *   has_ans           是否已经有上一次计算结果；没有时使用 Ans 会报错。
 *   result            输出参数；计算成功时保存结果。
 *   error_buffer      输出参数；计算失败时保存错误信息（可传 NULL）。
 *   error_buffer_size error_buffer 的可用字节数。
 *
 * 返回值：
 *   0   计算成功。
 *   -1  计算失败（语法错误、定义域错误、Ans 尚未产生等）。
 */
int calc_evaluate_with_ans(const char *expression,
                           double ans,
                           int has_ans,
                           double *result,
                           char *error_buffer,
                           size_t error_buffer_size);

/*
 * 计算表达式的值，允许使用 Ans，并指定三角函数角度制。
 *
 * 参数：
 *   mode              角度制：CALC_MODE_DEG / CALC_MODE_RAD / CALC_MODE_GRAD。
 *                     影响 sin/cos/tan 的输入解释；sind/cosd/tand 恒为角度制。
 *   其余参数同 calc_evaluate_with_ans。
 *
 * 返回值：
 *   0   计算成功。
 *   -1  计算失败。
 */
int calc_evaluate_mode(const char *expression,
                       CalcAngleMode mode,
                       double ans,
                       int has_ans,
                       double *result,
                       char *error_buffer,
                       size_t error_buffer_size);

/*
 * 复数求值 API：完整支持 j/i 虚数单位与复数结果。
 * 返回值、Ans 均为复数；可判断结果实部/虚部。
 */
int calc_evaluate_complex(const char *expression,
                          CalcAngleMode mode,
                          CalcComplex ans,
                          int has_ans,
                          CalcComplex *result,
                          char *error_buffer,
                          size_t error_buffer_size);

/* ------------------------------------------------------------------ */
/* 数值工具（v6.0）：以下函数把表达式看成以 x 为自变量的函数 f(x)。       */
/* 表达式内部可用字符 x 代表自变量，例如 "x^2 + 1"、"sin(x)"。           */
/* ------------------------------------------------------------------ */

/* 计算 f(x) 在给定 x 处的值（x 为实自变量，结果允许为复数）。 */
int calc_evaluate_x(const char *expression,
                    CalcAngleMode mode,
                    double x_value,
                    CalcComplex *result,
                    char *error_buffer,
                    size_t error_buffer_size);

/* 数值定积分：∫[a,b] f(x) dx（自适应 Gauss–Legendre 求积）。 */
int calc_ninteg(const char *expression,
                CalcAngleMode mode,
                double a,
                double b,
                CalcComplex *result,
                char *error_buffer,
                size_t error_buffer_size);

/* 数值导数 f'(x0)（四阶中心差分；返回复数，通常虚部≈0）。 */
int calc_nderiv(const char *expression,
                CalcAngleMode mode,
                double x0,
                CalcComplex *result,
                char *error_buffer,
                size_t error_buffer_size);

/* 在 [a,b] 内找 f(x)=0 的实根（二分法，需端点异号）。 */
int calc_root(const char *expression,
              CalcAngleMode mode,
              double a,
              double b,
              double *result,
              char *error_buffer,
              size_t error_buffer_size);

/* 求和：Σ_{i=a}^{b} f(i)（i 以 x 传入）。 */
int calc_sum(const char *expression,
             CalcAngleMode mode,
             long a,
             long b,
             CalcComplex *result,
             char *error_buffer,
             size_t error_buffer_size);

/* 连乘：Π_{i=a}^{b} f(i)（i 以 x 传入）。 */
int calc_prod(const char *expression,
              CalcAngleMode mode,
              long a,
              long b,
              CalcComplex *result,
              char *error_buffer,
              size_t error_buffer_size);

/* ------------------------------------------------------------------ */
/* 插件式函数注册表：每个算法库向引擎注册可被表达式调用的函数。          */
/* 这样各算法(矩阵/复数/…)编译成独立的动态库( .so / .dll )，互不依赖，  */
/* 单独修复某个算法时不影响其它部分。                                  */
/* ------------------------------------------------------------------ */

/* 一个被注册的算法函数：接收参数数组(个数为 nargs)、角度制 mode，
 * 可把错误写入 err；返回单精度结果。 */
typedef double (*calc_engine_fn)(const double *args, int nargs,
                                 CalcAngleMode mode,
                                 char *err, size_t errsz);

/* 函数描述：算法库通过它把“函数名+实现”交给主程序注册。 */
typedef struct {
    const char *name;
    calc_engine_fn fn;
} calc_function;

/* 向引擎注册一个可被表达式调用的函数（供主程序/算法库调用）。 */
void calc_register_function(const char *name, calc_engine_fn fn);

/* 批量注册一组函数描述（算法库返回的 calc_function 数组）。 */
void calc_register_functions(const calc_function *fns, size_t count);

#ifdef __cplusplus
}
#endif

#endif /* CALCULATOR_H */
