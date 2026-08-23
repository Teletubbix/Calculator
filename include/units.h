#ifndef UNITS_H
#define UNITS_H

#include <stddef.h>

/*
 * 单位换算：把 value 从 from 单位换算成 to 单位。
 * 支持长度/质量/温度/数据/时间/速度/功率/能量/频率/压强/dB 等常用单位。
 *
 * 参数：
 *   value   待换算的数值。
 *   from    源单位（名称或符号，大小写不敏感），如 "km"、"m"、"degC"、"dBm"。
 *   to      目标单位，如 "m"、"km"、"degF"、"mW"。
 *   out     输出参数，换算结果。
 *   err     错误信息缓冲（可传 NULL）。
 *   errsz   err 的字节数。
 *
 * 返回值：
 *   0   换算成功。
 *   -1  单位无法识别或不在同一类别，错误原因写入 err。
 */
int unit_convert(double value, const char *from, const char *to,
                 double *out, char *err, size_t errsz);

#endif /* UNITS_H */
