/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

#ifndef DB_H
#define DB_H

#include "calculator.h"   /* calc_function、CalcAngleMode */

/*
 * dB / 功率级转换算法库（独立动态库）。
 * 提供可直接在表达式里用的 dB 相关函数：
 *   dbm(mW)   功率(mW) -> dBm
 *   mw(dBm)   dBm -> 功率(mW)
 *   dbw(W)    功率(W) -> dBW
 *   w(dBW)    dBW -> 功率(W)
 *   pow2db(x) 功率比 -> dB （10·log10(x)）
 *   db2pow(dB) dB -> 功率比
 */
const calc_function *calc_db_functions(size_t *count);

#endif /* DB_H */
