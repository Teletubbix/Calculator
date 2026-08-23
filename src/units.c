/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

/*
 * 单位换算引擎
 * 线性单位：value * from.factor / to.factor
 * 温度：以摄氏度为核心，做偏移换算
 * dB：dBm/dBW 与 mW/W 之间的对数换算
 */

#include "units.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct { const char *sym; double factor; } LinUnit;

/* 每个 category 是一组换算到“基准单位”的线性因子 */
typedef struct { const char *name; const LinUnit *units; int n; } Category;

static int ci_equals(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static int find_lin(const LinUnit *units, int n, const char *sym, double *factor) {
    for (int i = 0; i < n; i++) {
        if (ci_equals(units[i].sym, sym)) { *factor = units[i].factor; return 1; }
    }
    return 0;
}

/* --- 各类别单位表（factor 为该单位换算到基准的倍数）--- */

static const LinUnit LENGTH[] = {
    {"mm", 0.001}, {"cm", 0.01}, {"dm", 0.1}, {"m", 1}, {"km", 1000},
    {"inch", 0.0254}, {"in", 0.0254}, {"ft", 0.3048}, {"foot", 0.3048},
    {"mile", 1609.344}, {"um", 1e-6}, {"nm", 1e-9}, {"mil", 2.54e-5},
};
static const LinUnit MASS[] = {
    {"mg", 1e-6}, {"g", 0.001}, {"kg", 1}, {"t", 1000}, {"ton", 1000},
    {"lb", 0.45359237}, {"oz", 0.0283495231},
};
static const LinUnit DATA[] = {
    {"bit", 0.125}, {"b", 0.125}, {"B", 1}, {"byte", 1},
    {"KB", 1000}, {"kB", 1000}, {"MB", 1e6}, {"GB", 1e9}, {"TB", 1e12},
    {"Kb", 125}, {"Mb", 1.25e5}, {"Gb", 1.25e8}, {"Tb", 1.25e11},
    {"KiB", 1024}, {"MiB", 1048576}, {"GiB", 1073741824}, {"TiB", 1099511627776},
};
static const LinUnit TIME[] = {
    {"ms", 0.001}, {"us", 1e-6}, {"ns", 1e-9}, {"s", 1}, {"sec", 1},
    {"min", 60}, {"h", 3600}, {"hr", 3600}, {"day", 86400}, {"week", 604800},
};
static const LinUnit SPEED[] = {
    {"mps", 1}, {"m/s", 1}, {"kmh", 1.0/3.6}, {"km/h", 1.0/3.6},
    {"mph", 0.44704}, {"knot", 0.514444}, {"c", 2.99792458e8},
};
static const LinUnit POWER[] = {
    {"mW", 0.001}, {"W", 1}, {"kW", 1000}, {"MW", 1e6}, {"hp", 745.699872},
};
static const LinUnit ENERGY[] = {
    {"J", 1}, {"kJ", 1000}, {"cal", 4.184}, {"kcal", 4184},
    {"Wh", 3600}, {"kWh", 3.6e6},
};
static const LinUnit FREQ[] = {
    {"Hz", 1}, {"kHz", 1e3}, {"MHz", 1e6}, {"GHz", 1e9}, {"THz", 1e12},
};
static const LinUnit PRESSURE[] = {
    {"Pa", 1}, {"kPa", 1000}, {"MPa", 1e6}, {"bar", 1e5},
    {"atm", 101325}, {"psi", 6894.757293168}, {"mmHg", 133.322387415},
};
static const LinUnit ANGLE[] = {
    {"deg", M_PI/180}, {"degree", M_PI/180}, {"rad", 1}, {"grad", M_PI/200},
    {"rev", 2*M_PI},
};

static const Category CATS[] = {
    {"length", LENGTH, (int)(sizeof LENGTH/sizeof LENGTH[0])},
    {"mass",   MASS,   (int)(sizeof MASS/sizeof MASS[0])},
    {"data",   DATA,   (int)(sizeof DATA/sizeof DATA[0])},
    {"time",   TIME,   (int)(sizeof TIME/sizeof TIME[0])},
    {"speed",  SPEED,  (int)(sizeof SPEED/sizeof SPEED[0])},
    {"power",  POWER,  (int)(sizeof POWER/sizeof POWER[0])},
    {"energy", ENERGY, (int)(sizeof ENERGY/sizeof ENERGY[0])},
    {"freq",   FREQ,   (int)(sizeof FREQ/sizeof FREQ[0])},
    {"pressure", PRESSURE, (int)(sizeof PRESSURE/sizeof PRESSURE[0])},
    {"angle",  ANGLE,  (int)(sizeof ANGLE/sizeof ANGLE[0])},
};

/* 温度：核心是摄氏度 */
typedef struct { const char *sym; double k; /* 该单位的 1 单位等于多少摄氏度（线性）*/ double c; /* 偏移 */} TempUnit;
static const TempUnit TEMP[] = {
    {"degC", 1, 0}, {"celsius", 1, 0}, {"c", 1, 0},
    {"degF", 5.0/9.0, -32*5.0/9.0}, {"fahrenheit", 5.0/9.0, -32*5.0/9.0}, {"f", 5.0/9.0, -32*5.0/9.0},
    {"K", 1, -273.15}, {"kelvin", 1, -273.15},
};
#define TEMP_N ((int)(sizeof TEMP/sizeof TEMP[0]))
static int is_temp(const char *s) {
    for (int i = 0; i < TEMP_N; i++) if (ci_equals(TEMP[i].sym, s)) return 1;
    return 0;
}
/* 把 v（单位 sym）换算成摄氏度 */
static double temp_to_c(const char *sym, double v) {
    for (int i = 0; i < TEMP_N; i++) if (ci_equals(TEMP[i].sym, sym)) return v * TEMP[i].k + TEMP[i].c;
    return v;
}
/* 把摄氏度换算成单位 sym */
static double c_to_temp(const char *sym, double c) {
    for (int i = 0; i < TEMP_N; i++) if (ci_equals(TEMP[i].sym, sym)) return (c - TEMP[i].c) / TEMP[i].k;
    return c;
}

/* dB：核心是 mW */
static int to_mw(const char *sym, double v, double *mw) {
    if (ci_equals(sym, "mW")) { *mw = v; return 1; }
    if (ci_equals(sym, "W"))  { *mw = v * 1000.0; return 1; }
    if (ci_equals(sym, "dBm")) { *mw = pow(10.0, v / 10.0); return 1; }
    if (ci_equals(sym, "dBW")) { *mw = pow(10.0, v / 10.0) * 1000.0; return 1; }
    return 0;
}
static int from_mw(const char *sym, double mw, double *out) {
    if (ci_equals(sym, "mW")) { *out = mw; return 1; }
    if (ci_equals(sym, "W"))  { *out = mw / 1000.0; return 1; }
    if (ci_equals(sym, "dBm")) { *out = 10.0 * log10(mw); return 1; }
    if (ci_equals(sym, "dBW")) { *out = 10.0 * log10(mw / 1000.0); return 1; }
    return 0;
}
static int is_db(const char *s) {
    return ci_equals(s, "dBm") || ci_equals(s, "dBW");
}

int unit_convert(double value, const char *from, const char *to,
                 double *out, char *err, size_t errsz) {
    if (from == NULL || to == NULL || out == NULL) return -1;
    if (err != NULL && errsz > 0) err[0] = '\0';

    /* 温度 */
    if (is_temp(from) || is_temp(to)) {
        if (!is_temp(from) || !is_temp(to)) {
            if (err) snprintf(err, errsz, "温度单位不能和非温度单位混算");
            return -1;
        }
        double c = temp_to_c(from, value);
        *out = c_to_temp(to, c);
        return 0;
    }
    /* dB：仅当其中一个是 dBm/dBW 时走对数换算 */
    if (is_db(from) || is_db(to)) {
        double mw = 0, r = 0;
        if (!to_mw(from, value, &mw) || !from_mw(to, mw, &r)) {
            if (err) snprintf(err, errsz, "无法识别功率/dB 单位");
            return -1;
        }
        *out = r;
        return 0;
    }
    /* 线性：在同一个类别里找 from 和 to */
    for (size_t i = 0; i < sizeof CATS/sizeof CATS[0]; i++) {
        double ff = 0, tf = 0;
        if (find_lin(CATS[i].units, CATS[i].n, from, &ff) &&
            find_lin(CATS[i].units, CATS[i].n, to, &tf)) {
            *out = value * ff / tf;
            return 0;
        }
    }
    if (err) snprintf(err, errsz, "无法识别单位：'%s' 或 '%s'（两者须在同一类别）", from, to);
    return -1;
}
