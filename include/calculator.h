#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * 计算一个数学表达式的值。
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

#ifdef __cplusplus
}
#endif

#endif /* CALCULATOR_H */
