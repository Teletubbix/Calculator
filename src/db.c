/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

/*
 * dB / 功率级算法库（独立动态库）：通信中最常用的功率换算。
 * 只定义函数并把描述交给主程序注册，不链接核心引擎。
 */

#include "db.h"

#include <math.h>
#include <stddef.h>
#include <stdio.h>

static double d_dbm(const double *a, int n, CalcAngleMode mode,
                    char *err, size_t es) {
    (void)mode;
    if (n != 1) { if (es) snprintf(err, es, "dbm 需要 1 个参数（功率 mW）"); return 0.0; }
    if (a[0] < 0) { if (es) snprintf(err, es, "dbm 需要非负功率（mW）"); return 0.0; }
    if (a[0] == 0) return -HUGE_VAL;   /* 0 mW = -inf dBm */
    return 10.0 * log10(a[0]);
}

static double d_mw(const double *a, int n, CalcAngleMode mode,
                   char *err, size_t es) {
    (void)mode;
    if (n != 1) { if (es) snprintf(err, es, "mw 需要 1 个参数（dBm）"); return 0.0; }
    return pow(10.0, a[0] / 10.0);
}

static double d_dbw(const double *a, int n, CalcAngleMode mode,
                    char *err, size_t es) {
    (void)mode;
    if (n != 1) { if (es) snprintf(err, es, "dbw 需要 1 个参数（功率 W）"); return 0.0; }
    if (a[0] < 0) { if (es) snprintf(err, es, "dbw 需要非负功率（W）"); return 0.0; }
    if (a[0] == 0) return -HUGE_VAL;
    return 10.0 * log10(a[0]);
}

static double d_w(const double *a, int n, CalcAngleMode mode,
                  char *err, size_t es) {
    (void)mode;
    if (n != 1) { if (es) snprintf(err, es, "w 需要 1 个参数（dBW）"); return 0.0; }
    return pow(10.0, a[0] / 10.0);
}

static double d_pow2db(const double *a, int n, CalcAngleMode mode,
                       char *err, size_t es) {
    (void)mode;
    if (n != 1) { if (es) snprintf(err, es, "pow2db 需要 1 个参数（功率比）"); return 0.0; }
    if (a[0] <= 0.0) { if (es) snprintf(err, es, "pow2db 需要正的功率比"); return 0.0; }
    return 10.0 * log10(a[0]);
}

static double d_db2pow(const double *a, int n, CalcAngleMode mode,
                       char *err, size_t es) {
    (void)mode;
    if (n != 1) { if (es) snprintf(err, es, "db2pow 需要 1 个参数（dB）"); return 0.0; }
    return pow(10.0, a[0] / 10.0);
}

static const calc_function g_db_fns[] = {
    {"dbm",    d_dbm},
    {"mw",     d_mw},
    {"dbw",    d_dbw},
    {"w",      d_w},
    {"pow2db", d_pow2db},
    {"db2pow", d_db2pow},
};

const calc_function *calc_db_functions(size_t *count) {
    if (count != NULL) *count = sizeof(g_db_fns) / sizeof(g_db_fns[0]);
    return g_db_fns;
}
