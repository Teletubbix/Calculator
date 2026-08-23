/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

/*
 * 矩阵库（独立动态库）：方阵行列式与迹。
 * 只定义函数并把描述交给主程序注册，不链接核心引擎。
 */

#include "matrix.h"

#include <stdio.h>
#include <stddef.h>

static double m_det2(const double *a, int n, CalcAngleMode mode,
                     char *err, size_t es) {
    (void)mode;
    if (n != 4) { if (es) snprintf(err, es, "det2 需要 4 个参数：det2(a,b,c,d)"); return 0.0; }
    return a[0] * a[3] - a[1] * a[2];
}

static double m_trace2(const double *a, int n, CalcAngleMode mode,
                       char *err, size_t es) {
    (void)mode;
    if (n != 4) { if (es) snprintf(err, es, "trace2 需要 4 个参数：trace2(a,b,c,d)"); return 0.0; }
    return a[0] + a[3];
}

static double m_det3(const double *a, int n, CalcAngleMode mode,
                     char *err, size_t es) {
    (void)mode;
    if (n != 9) { if (es) snprintf(err, es, "det3 需要 9 个参数（按行优先）"); return 0.0; }
    return a[0] * (a[4] * a[8] - a[5] * a[7])
         - a[1] * (a[3] * a[8] - a[5] * a[6])
         + a[2] * (a[3] * a[7] - a[4] * a[6]);
}

static double m_trace3(const double *a, int n, CalcAngleMode mode,
                       char *err, size_t es) {
    (void)mode;
    if (n != 9) { if (es) snprintf(err, es, "trace3 需要 9 个参数（按行优先）"); return 0.0; }
    return a[0] + a[4] + a[8];
}

static const calc_function g_matrix_fns[] = {
    {"det2",   m_det2},
    {"trace2", m_trace2},
    {"det3",   m_det3},
    {"trace3", m_trace3},
};

const calc_function *calc_matrix_functions(size_t *count) {
    if (count != NULL) *count = sizeof(g_matrix_fns) / sizeof(g_matrix_fns[0]);
    return g_matrix_fns;
}
