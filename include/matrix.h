/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

#ifndef MATRIX_H
#define MATRIX_H

#include "calculator.h"   /* 仅使用 calc_function 类型，不依赖核心实现 */

/*
 * 返回本库注册给引擎的矩阵函数描述数组（det2/det3/trace2/trace3）。
 * 主程序用 calc_register_functions() 注册它们。矩阵库本身不链接核心，
 * 只定义函数，因此可与其它算法库独立编译、独立修复。
 */
const calc_function *calc_matrix_functions(size_t *count);

#endif /* MATRIX_H */
