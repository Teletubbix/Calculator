#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 计算一个数学表达式的值（不含 Ans，旧版兼容接口）。
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

#ifdef __cplusplus
}
#endif

#endif /* CALCULATOR_H */
