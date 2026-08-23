#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stddef.h>

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
