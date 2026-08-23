/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

/*
 * Calculator 表达式求值核心
 *
 * 支持：
 *   - 基本运算：+ - * / ^ !
 *   - 函数：sin cos tan（弧度）、sind cosd tand（角度）
 *           sqrt、ln、log（底数为 10）、log2、pow
 *   - 常量：pi / π、e（不区分大小写）
 *   - 括号、一元正负号、小数、科学计数法
 *
 * 语法（优先级从低到高）：
 *   expression -> term (('+' | '-') term)*
 *   term       -> unary (('*' | '/') unary)*
 *   unary      -> ('+' | '-') unary | power
 *   power      -> postfix ('^' unary)?      // 幂运算右结合
 *   postfix    -> primary '!'?              // 阶乘是后缀运算
 *   primary    -> 数字 | 常量 | 函数(...) | '(' expression ')'
 */

#include "calculator.h"
#include "complex.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_E
#define M_E 2.71828182845904523536
#endif

#define MAX_IDENT_LEN 31
#define MAX_ERROR_LEN 255

/* ------------------------------------------------------------------ */
/* 词法分析：把字符串拆成数字、标识符和运算符                           */
/* ------------------------------------------------------------------ */

typedef enum {
    TOK_END = 0,
    TOK_NUMBER,
    TOK_IDENT,
    TOK_PLUS,    /* + */
    TOK_MINUS,   /* - */
    TOK_STAR,    /* * */
    TOK_SLASH,   /* / */
    TOK_CARET,   /* ^ */
    TOK_BANG,    /* ! */
    TOK_LPAREN,  /* ( */
    TOK_RPAREN,  /* ) */
    TOK_COMMA,   /* , */
    TOK_ANGLE,   /* ∠ 或 @：极坐标相量 r∠θ */
    TOK_ERROR
} TokenType;

typedef struct {
    const char *src;      /* 待分析的表达式 */
    int pos;              /* 下一个待读取字符的下标（按字节计） */
    int token_pos;        /* 当前 token 的起始下标，用于错误定位 */
    TokenType type;
    double number;
    char ident[MAX_IDENT_LEN + 1];
} Lexer;

static void lexer_init(Lexer *lx, const char *src) {
    lx->src = src;
    lx->pos = 0;
    lx->token_pos = 0;
    lx->type = TOK_ERROR;
    lx->number = 0.0;
    lx->ident[0] = '\0';
}

static void lexer_next(Lexer *lx) {
    const char *src = lx->src;

    /* 跳过所有空白字符 */
    while (isspace((unsigned char)src[lx->pos])) {
        lx->pos++;
    }

    lx->token_pos = lx->pos;
    lx->ident[0] = '\0';
    unsigned char c = (unsigned char)src[lx->pos];

    if (c == '\0') {
        lx->type = TOK_END;
        return;
    }

    /* 数字：允许 3、3.14、.5、1e-3 这类写法 */
    if (isdigit(c) || (c == '.' && isdigit((unsigned char)src[lx->pos + 1]))) {
        char *end = NULL;
        lx->number = strtod(src + lx->pos, &end);
        if (end == src + lx->pos) {
            lx->type = TOK_ERROR;
            return;
        }
        lx->pos = (int)(end - src);
        lx->type = TOK_NUMBER;
        return;
    }

    /* 标识符：函数名（sin、cos...）和常量名（pi、e） */
    if (isalpha(c) || c == '_') {
        int i = 0;
        while ((isalnum((unsigned char)src[lx->pos]) ||
                src[lx->pos] == '_') &&
               i < MAX_IDENT_LEN) {
            lx->ident[i++] = src[lx->pos++];
        }
        lx->ident[i] = '\0';
        lx->type = TOK_IDENT;
        return;
    }

    /* 希腊字母 π（UTF-8: 0xCF 0x80）当作常量 pi */
    if (c == 0xCF && (unsigned char)src[lx->pos + 1] == 0x80) {
        strcpy(lx->ident, "pi");
        lx->pos += 2;
        lx->type = TOK_IDENT;
        return;
    }

    /* ∠（U+2220, UTF-8: E2 88 A0）—— 极坐标相量运算符 */
    if (c == 0xE2 && (unsigned char)src[lx->pos + 1] == 0x88 &&
        (unsigned char)src[lx->pos + 2] == 0xA0) {
        lx->pos += 3;
        lx->type = TOK_ANGLE;
        return;
    }

    /* 单字符运算符 */
    lx->pos++;
    switch (c) {
        case '+': lx->type = TOK_PLUS;   return;
        case '-': lx->type = TOK_MINUS;  return;
        case '*': lx->type = TOK_STAR;   return;
        case '/': lx->type = TOK_SLASH;  return;
        case '^': lx->type = TOK_CARET;  return;
        case '!': lx->type = TOK_BANG;   return;
        case '(': lx->type = TOK_LPAREN; return;
        case ')': lx->type = TOK_RPAREN; return;
        case ',': lx->type = TOK_COMMA;  return;
        case '@': lx->type = TOK_ANGLE;  return;  /* ASCII 别名：r@θ */
        default:  lx->type = TOK_ERROR;  return;
    }
}

/* ------------------------------------------------------------------ */
/* 递归下降语法分析 + 求值                                             */
/* ------------------------------------------------------------------ */

typedef struct {
    Lexer lx;
    int failed;               /* 是否已经发生错误 */
    int error_pos;            /* 错误位置（字节下标） */
    char message[MAX_ERROR_LEN + 1];
    CalcComplex ans_value;         /* 上一次计算结果，供 Ans 使用 */
    int has_ans;              /* 是否已经有上一次计算结果 */
    CalcAngleMode mode;       /* 三角函数角度制 */
} Parser;

static void parser_set_error(Parser *p, int pos, const char *fmt, ...) {
    if (p->failed) {
        return; /* 只保留第一个错误 */
    }
    p->failed = 1;
    p->error_pos = pos;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(p->message, sizeof(p->message), fmt, ap);
    va_end(ap);
}

static int ident_equals(const char *ident, const char *name) {
    while (*ident && *name) {
        if (tolower((unsigned char)*ident) != tolower((unsigned char)*name)) {
            return 0;
        }
        ident++;
        name++;
    }
    return *ident == '\0' && *name == '\0';
}

static CalcComplex parse_expression(Parser *p);
static CalcComplex parse_term(Parser *p);
static CalcComplex parse_unary(Parser *p);
static CalcComplex parse_angle(Parser *p);
static CalcComplex parse_power(Parser *p);
static CalcComplex parse_postfix(Parser *p);
static CalcComplex parse_primary(Parser *p);

/* 便捷构造：实数 */
static CalcComplex zc(double re) { CalcComplex z = { re, 0.0 }; return z; }

static double angle_to_rad(Parser *p, double x);   /* 前向声明 */

/* 二元运算（复数），并做定义域检查 */
static CalcComplex binary_op(Parser *p, int pos, char op, CalcComplex a, CalcComplex b) {
    CalcComplex r;
    switch (op) {
        case '+': r = calc_c_add(a, b); break;
        case '-': r = calc_c_sub(a, b); break;
        case '*': r = calc_c_mul(a, b); break;
        case '/':
            if (b.re == 0.0 && b.im == 0.0) {
                parser_set_error(p, pos, "除法错误：除数不能为 0");
                return zc(0.0);
            }
            r = calc_c_div(a, b);
            break;
        case '^':
            r = calc_c_pow(a, b);
            if (isnan(r.re) || isnan(r.im)) {
                parser_set_error(p, pos, "乘方结果无定义（如 0 的负次幂）");
                return zc(0.0);
            }
            break;
        case '@':   /* 极坐标相量 r∠θ（θ 按当前角度制解释，等义于 polar(a, b)）*/
            if (b.im != 0.0) {
                parser_set_error(p, pos, "极坐标角度应为实数");
                return zc(0.0);
            }
            r = calc_c_from_polar(a.re, angle_to_rad(p, b.re));
            break;
        default:
            parser_set_error(p, pos, "内部错误：未知运算符 '%c'", op);
            return zc(0.0);
    }
    /* 溢出检测：结果非有限 */
    if (!isfinite(r.re) || !isfinite(r.im)) {
        parser_set_error(p, pos, "计算结果超出 double 可表示的范围");
        return zc(0.0);
    }
    return r;
}

/* 按当前角度制把输入角换算成弧度 */
static double angle_to_rad(Parser *p, double x) {
    switch (p->mode) {
        case CALC_MODE_DEG:  return x * M_PI / 180.0;
        case CALC_MODE_GRAD: return x * M_PI / 200.0;
        case CALC_MODE_RAD:
        default:             return x;
    }
}

/* 把弧度换算成当前角度制下的值（用于反三角函数的返回值） */
static double rad_to_angle(Parser *p, double x) {
    switch (p->mode) {
        case CALC_MODE_DEG:  return x * 180.0 / M_PI;
        case CALC_MODE_GRAD: return x * 200.0 / M_PI;
        case CALC_MODE_RAD:
        default:             return x;
    }
}

/* 一元函数（复数），并做定义域检查 */
static CalcComplex unary_function(Parser *p, int pos, const char *name, CalcComplex x) {
    /* 三角函数（受角度制影响；复数输入按弧度处理） */
    if (ident_equals(name, "sin")) {
        if (x.im == 0.0) return calc_c_sin(zc(angle_to_rad(p, x.re)));
        return calc_c_sin(x);
    }
    if (ident_equals(name, "cos")) {
        if (x.im == 0.0) return calc_c_cos(zc(angle_to_rad(p, x.re)));
        return calc_c_cos(x);
    }
    if (ident_equals(name, "tan")) {
        if (x.im == 0.0) return calc_c_tan(zc(angle_to_rad(p, x.re)));
        return calc_c_tan(x);
    }

    /* 恒为角度制的三角函数：只接受实数输入 */
    if (ident_equals(name, "sind") || ident_equals(name, "cosd") || ident_equals(name, "tand")) {
        if (x.im != 0.0) { parser_set_error(p, pos, "函数 '%s' 只接受实数角度", name); return zc(0.0); }
        double rad = x.re * M_PI / 180.0;
        if (ident_equals(name, "sind")) return zc(sin(rad));
        if (ident_equals(name, "cosd")) return zc(cos(rad));
        return zc(tan(rad));
    }

    /* 反三角：实数输入按角度模式输出；复数输入给主值（弧度） */
    if (ident_equals(name, "asin")) {
        CalcComplex r = calc_c_asin(x);
        if (r.im == 0.0) return zc(rad_to_angle(p, r.re));
        return r;
    }
    if (ident_equals(name, "acos")) {
        CalcComplex r = calc_c_acos(x);
        if (r.im == 0.0) return zc(rad_to_angle(p, r.re));
        return r;
    }
    if (ident_equals(name, "atan")) {
        CalcComplex r = calc_c_atan(x);
        if (r.im == 0.0) return zc(rad_to_angle(p, r.re));
        return r;
    }
    /* 反三角（角度输出）：只接受实数 */
    if (ident_equals(name, "asind") || ident_equals(name, "acosd") || ident_equals(name, "atand")) {
        if (x.im != 0.0) { parser_set_error(p, pos, "函数 '%s' 只接受实数", name); return zc(0.0); }
        CalcComplex r;
        if (ident_equals(name, "asind")) r = calc_c_asin(x);
        else if (ident_equals(name, "acosd")) r = calc_c_acos(x);
        else r = calc_c_atan(x);
        if (r.im != 0.0) { parser_set_error(p, pos, "函数 '%s' 输入超出定义域", name); return zc(0.0); }
        return zc(r.re * 180.0 / M_PI);
    }

    /* 双曲函数 */
    if (ident_equals(name, "sinh")) return calc_c_sinh(x);
    if (ident_equals(name, "cosh")) return calc_c_cosh(x);
    if (ident_equals(name, "tanh")) return calc_c_tanh(x);

    /* 复数友好的开根/对数/指数 */
    if (ident_equals(name, "sqrt")) return calc_c_sqrt(x);
    if (ident_equals(name, "ln"))   return calc_c_log(x);
    if (ident_equals(name, "log"))  return calc_c_mul(zc(1.0 / log(10.0)), calc_c_log(x)); /* log10 */
    if (ident_equals(name, "log2")) return calc_c_mul(zc(1.0 / log(2.0)),  calc_c_log(x));
    if (ident_equals(name, "exp"))  return calc_c_exp(x);
    if (ident_equals(name, "abs"))  return zc(calc_c_abs(x));    /* 模（实数） */

    /* 实数专用函数 */
    if (ident_equals(name, "sign")) {
        if (x.im != 0.0) { parser_set_error(p, pos, "sign 只接受实数"); return zc(0.0); }
        return zc((x.re > 0.0) ? 1.0 : (x.re < 0.0) ? -1.0 : 0.0);
    }
    if (ident_equals(name, "floor") || ident_equals(name, "ceil") ||
        ident_equals(name, "round") || ident_equals(name, "trunc")) {
        if (x.im != 0.0) { parser_set_error(p, pos, "函数 '%s' 只接受实数", name); return zc(0.0); }
        if (ident_equals(name, "floor")) return zc(floor(x.re));
        if (ident_equals(name, "ceil"))  return zc(ceil(x.re));
        if (ident_equals(name, "round")) return zc(round(x.re));
        return zc(trunc(x.re));
    }

    parser_set_error(p, pos, "未知函数 '%s'", name);
    return zc(0.0);
}

/* 二元函数（两个参数），做定义域检查 */
static CalcComplex binary_function(Parser *p, int pos, const char *name, CalcComplex a, CalcComplex b) {
    if (ident_equals(name, "atan2")) {
        if (a.im != 0.0 || b.im != 0.0) { parser_set_error(p, pos, "atan2 只接受实数"); return zc(0.0); }
        return zc(rad_to_angle(p, atan2(a.re, b.re)));
    }
    if (ident_equals(name, "mod")) {
        if (a.im != 0.0 || b.im != 0.0) { parser_set_error(p, pos, "mod 只接受实数"); return zc(0.0); }
        if (b.re == 0.0) { parser_set_error(p, pos, "mod 错误：模数不能为 0"); return zc(0.0); }
        return zc(fmod(a.re, b.re));
    }
    if (ident_equals(name, "gcd")) {
        if (a.im != 0.0 || b.im != 0.0) { parser_set_error(p, pos, "gcd 只接受实数"); return zc(0.0); }
        long long x = (long long)llround(a.re), y = (long long)llround(b.re);
        if (a.re < 0.0 || b.re < 0.0 || (double)x != a.re || (double)y != b.re) {
            parser_set_error(p, pos, "gcd 要求两个非负整数，例如 gcd(12,18)"); return zc(0.0);
        }
        while (y != 0) { long long t = x % y; x = y; y = t; }
        return zc((double)x);
    }
    if (ident_equals(name, "lcm")) {
        if (a.im != 0.0 || b.im != 0.0) { parser_set_error(p, pos, "lcm 只接受实数"); return zc(0.0); }
        long long x = (long long)llround(a.re), y = (long long)llround(b.re);
        if (a.re < 0.0 || b.re < 0.0 || (double)x != a.re || (double)y != b.re || (x == 0 && y == 0)) {
            parser_set_error(p, pos, "lcm 要求两个非负整数，例如 lcm(4,6)"); return zc(0.0);
        }
        long long g = x, h = y;
        while (h != 0) { long long t = g % h; g = h; h = t; }
        return zc((double)(x / g * y));
    }
    if (ident_equals(name, "comb")) {
        if (a.im != 0.0 || b.im != 0.0) { parser_set_error(p, pos, "comb 只接受实数"); return zc(0.0); }
        long long n = (long long)llround(a.re), k = (long long)llround(b.re);
        if (a.re < 0.0 || b.re < 0.0 || (double)n != a.re || (double)k != b.re || k > n || n > 60.0) {
            parser_set_error(p, pos, "comb 要求 n>=k>=0 且 n<=60，例如 comb(10,3)"); return zc(0.0);
        }
        if (k > n - k) k = n - k;
        double r = 1.0;
        for (long long i = 1; i <= k; i++) r = r * (double)(n - k + i) / (double)i;
        return zc(r);
    }
    if (ident_equals(name, "perm")) {
        if (a.im != 0.0 || b.im != 0.0) { parser_set_error(p, pos, "perm 只接受实数"); return zc(0.0); }
        long long n = (long long)llround(a.re), k = (long long)llround(b.re);
        if (a.re < 0.0 || b.re < 0.0 || (double)n != a.re || (double)k != b.re || k > n || n > 170.0) {
            parser_set_error(p, pos, "perm 要求 n>=k>=0 且 n<=170，例如 perm(10,3)"); return zc(0.0);
        }
        double r = 1.0;
        for (long long i = 0; i < k; i++) r *= (double)(n - i);
        return zc(r);
    }
    if (ident_equals(name, "logn")) {
        if (a.im != 0.0 || b.im != 0.0) { parser_set_error(p, pos, "logn 只接受实数"); return zc(0.0); }
        if (a.re <= 0.0 || b.re <= 1.0) {
            parser_set_error(p, pos, "logn 定义域错误：真数>0 且底数>1"); return zc(0.0);
        }
        return zc(log(a.re) / log(b.re));
    }
    parser_set_error(p, pos, "未知二元函数 '%s'", name);
    return zc(0.0);
}

/* 阶乘：只对 0~170 的非负整数有定义 */
static CalcComplex factorial_op(Parser *p, int pos, CalcComplex x) {
    if (x.im != 0.0) {
        parser_set_error(p, pos, "阶乘错误：n! 要求 n 是非负整数");
        return zc(0.0);
    }
    double v = x.re;
    if (v < 0.0 || floor(v) != v) {
        parser_set_error(p, pos, "阶乘错误：n! 要求 n 是非负整数（例如 5!）");
        return zc(0.0);
    }
    if (v > 170.0) {
        parser_set_error(p, pos, "阶乘错误：n 太大，171! 已超出 double 可表示的范围");
        return zc(0.0);
    }
    double r = 1.0;
    for (int i = 2; i <= (int)v; i++) r *= (double)i;
    return zc(r);
}

/* ------------------------------------------------------------------ */
/* 插件式函数注册表                                                     */
/* ------------------------------------------------------------------ */

#define MAX_REGISTERED 64
static struct { const char *name; calc_engine_fn fn; } g_fns[MAX_REGISTERED];
static int g_fn_count = 0;

void calc_register_function(const char *name, calc_engine_fn fn) {
    if (name == NULL || fn == NULL) return;
    for (int i = 0; i < g_fn_count; i++) {
        if (strcmp(g_fns[i].name, name) == 0) { g_fns[i].fn = fn; return; } /* 幂等：覆盖同名 */
    }
    if (g_fn_count < MAX_REGISTERED) {
        g_fns[g_fn_count].name = name;
        g_fns[g_fn_count].fn = fn;
        g_fn_count++;
    }
}

void calc_register_functions(const calc_function *fns, size_t count) {
    if (fns == NULL) return;
    for (size_t i = 0; i < count; i++) {
        calc_register_function(fns[i].name, fns[i].fn);
    }
}

static calc_engine_fn registry_lookup(const char *name) {
    for (int i = 0; i < g_fn_count; i++) {
        if (ident_equals(g_fns[i].name, name)) return g_fns[i].fn;
    }
    return NULL;
}

/*
 * expression -> term (('+' | '-') term)*
 */
static CalcComplex parse_expression(Parser *p) {
    CalcComplex left = parse_term(p);
    if (p->failed) {
        return zc(0.0);
    }
    while (p->lx.type == TOK_PLUS || p->lx.type == TOK_MINUS) {
        char op = (p->lx.type == TOK_PLUS) ? '+' : '-';
        int op_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        CalcComplex right = parse_term(p);
        if (p->failed) {
            return zc(0.0);
        }
        left = binary_op(p, op_pos, op, left, right);
        if (p->failed) {
            return zc(0.0);
        }
    }
    return left;
}

/*
 * term -> unary (('*' | '/') unary)*
 */
static CalcComplex parse_term(Parser *p) {
    CalcComplex left = parse_unary(p);
    if (p->failed) {
        return zc(0.0);
    }
    while (p->lx.type == TOK_STAR || p->lx.type == TOK_SLASH) {
        char op = (p->lx.type == TOK_STAR) ? '*' : '/';
        int op_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        CalcComplex right = parse_unary(p);
        if (p->failed) {
            return zc(0.0);
        }
        left = binary_op(p, op_pos, op, left, right);
        if (p->failed) {
            return zc(0.0);
        }
    }
    return left;
}

/*
 * unary -> ('+' | '-') unary | power
 *
 * 这样设计后，-3^2 按数学惯例等于 -(3^2) = -9；
 * 如果要计算 (-3)^2 = 9，请显式加括号。
 */
static CalcComplex parse_unary(Parser *p) {
    if (p->lx.type == TOK_PLUS || p->lx.type == TOK_MINUS) {
        char sign = (p->lx.type == TOK_PLUS) ? '+' : '-';
        lexer_next(&p->lx);
        CalcComplex value = parse_unary(p);
        if (p->failed) {
            return zc(0.0);
        }
        if (sign == '-') {
            return calc_c_mul(zc(-1.0), value);
        }
        return value;
    }
    return parse_angle(p);
}

/*
 * angle -> power ('∠' power)?    // 极坐标相量 r∠θ（@ 是 ASCII 别名）
 * ∠ 的优先级高于 * /，因此 5∠30*2 = (5∠30)*2
 */
static CalcComplex parse_angle(Parser *p) {
    CalcComplex mag = parse_power(p);
    if (p->failed) {
        return zc(0.0);
    }
    if (p->lx.type == TOK_ANGLE) {
        int op_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        CalcComplex ang = parse_power(p);
        if (p->failed) {
            return zc(0.0);
        }
        return binary_op(p, op_pos, '@', mag, ang);
    }
    return mag;
}

/*
 * power -> postfix ('^' unary)?
 *
 * 幂运算右结合：2^3^2 = 2^(3^2) = 512。
 * 指数部分复用 unary，因此允许 2^-2 这类写法。
 */
static CalcComplex parse_power(Parser *p) {
    CalcComplex base = parse_postfix(p);
    if (p->failed) {
        return zc(0.0);
    }
    if (p->lx.type == TOK_CARET) {
        int op_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        CalcComplex exponent = parse_unary(p);
        if (p->failed) {
            return zc(0.0);
        }
        return binary_op(p, op_pos, '^', base, exponent);
    }
    return base;
}

/*
 * postfix -> primary '!'?
 */
static CalcComplex parse_postfix(Parser *p) {
    CalcComplex value = parse_primary(p);
    if (p->failed) {
        return zc(0.0);
    }
    if (p->lx.type == TOK_BANG) {
        int bang_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        value = factorial_op(p, bang_pos, value);
    }
    return value;
}

/*
 * primary -> 数字
 *          | 常量（pi / π / e）
 *          | 函数 '(' 参数列表 ')'
 *          | '(' expression ')'
 */
static CalcComplex parse_primary(Parser *p) {
    if (p->lx.type == TOK_NUMBER) {
        double num = p->lx.number;
        lexer_next(&p->lx);
        /* 数字后紧跟 j/i 视为虚数，如 4j、2i */
        if (p->lx.type == TOK_IDENT &&
            (ident_equals(p->lx.ident, "j") || ident_equals(p->lx.ident, "i"))) {
            lexer_next(&p->lx);
            CalcComplex u = { 0.0, num };
            return u;
        }
        return zc(num);
    }

    if (p->lx.type == TOK_IDENT) {
        char name[MAX_IDENT_LEN + 1];
        strncpy(name, p->lx.ident, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        int ident_pos = p->lx.token_pos;
        lexer_next(&p->lx);

        if (p->lx.type == TOK_LPAREN) {
            lexer_next(&p->lx); /* 跳过 '(' */

            /* 解析参数列表（逗号分隔）。二元/矩阵函数按参数个数分派 */
            CalcComplex args[9];
            int nargs = 0;
            while (1) {
                if (nargs >= 9) {
                    parser_set_error(p, p->lx.token_pos,
                                     "函数 '%s' 参数过多（最多 9 个）", name);
                    return zc(0.0);
                }
                args[nargs++] = parse_expression(p);
                if (p->failed) {
                    return zc(0.0);
                }
                if (p->lx.type == TOK_COMMA) {
                    lexer_next(&p->lx);
                    continue;
                }
                break;
            }
            if (p->lx.type != TOK_RPAREN) {
                parser_set_error(p, p->lx.token_pos, "函数 '%s' 缺少右括号 ')'", name);
                return zc(0.0);
            }
            lexer_next(&p->lx);

            /* 先在注册表中查找（算法库注册的矩阵/复数等函数） */
            calc_engine_fn reg = registry_lookup(name);
            if (reg != NULL) {
                double dargs[9];
                for (int i = 0; i < nargs; i++) {
                    if (args[i].im != 0.0) {
                        parser_set_error(p, ident_pos, "函数 '%s' 需要实数参数", name);
                        return zc(0.0);
                    }
                    dargs[i] = args[i].re;
                }
                char rerr[256] = {0};
                double r = reg(dargs, nargs, p->mode, rerr, sizeof(rerr));
                if (rerr[0] != '\0') {
                    parser_set_error(p, ident_pos, "%s", rerr);
                    return zc(0.0);
                }
                return zc(r);
            }

            if (nargs == 1) {
                return unary_function(p, ident_pos, name, args[0]);
            }
            if (nargs == 2) {
                if (ident_equals(name, "pow")) {
                    return binary_op(p, ident_pos, '^', args[0], args[1]);
                }
                return binary_function(p, ident_pos, name, args[0], args[1]);
            }
            parser_set_error(p, p->lx.token_pos, "函数 '%s' 参数个数不正确", name);
            return zc(0.0);
        }

        /* 不是函数，就按常量处理 */
        if (ident_equals(name, "pi"))   return zc(M_PI);
        if (ident_equals(name, "e"))    return zc(M_E);
        if (ident_equals(name, "tau"))  return zc(2.0 * M_PI);
        if (ident_equals(name, "phi"))  return zc((1.0 + sqrt(5.0)) / 2.0);
        if (ident_equals(name, "j") || ident_equals(name, "i")) {
            CalcComplex u = { 0.0, 1.0 };   /* 虚数单位 j/i */
            return u;
        }
        if (ident_equals(name, "ans")) {
            if (!p->has_ans) {
                parser_set_error(p, ident_pos,
                                 "还没有上一次计算结果，请先完成一次计算再使用 Ans");
                return zc(0.0);
            }
            return p->ans_value;
        }
        parser_set_error(p, ident_pos, "未知常量 '%s'（可用常量：pi、e、tau、phi、j、Ans）", name);
        return zc(0.0);
    }

    if (p->lx.type == TOK_LPAREN) {
        lexer_next(&p->lx);
        CalcComplex value = parse_expression(p);
        if (p->failed) {
            return zc(0.0);
        }
        if (p->lx.type != TOK_RPAREN) {
            parser_set_error(p, p->lx.token_pos, "缺少右括号 ')'");
            return zc(0.0);
        }
        lexer_next(&p->lx);
        return value;
    }

    if (p->lx.type == TOK_ERROR) {
        parser_set_error(p, p->lx.token_pos, "存在无法识别的字符或数字格式");
        return zc(0.0);
    }

    parser_set_error(p, p->lx.token_pos, "此处应为数字、常量、函数或左括号 '('");
    return zc(0.0);
}

/* ------------------------------------------------------------------ */
/* 对外接口                                                             */
/* ------------------------------------------------------------------ */

static size_t append_text(char *out, size_t out_size, size_t used, const char *text) {
    if (out == NULL || out_size == 0 || used >= out_size) {
        return used;
    }
    int written = snprintf(out + used, out_size - used, "%s", text);
    if (written > 0) {
        used += (size_t)written;
    }
    return used;
}

/*
 * 把错误信息和“箭头定位”格式化成易读文本。
 */
static void format_error(const char *message,
                         int pos,
                         const char *expression,
                         char *out,
                         size_t out_size) {
    size_t used = 0;
    used = append_text(out, out_size, used, message);
    used = append_text(out, out_size, used, "\n表达式：");
    used = append_text(out, out_size, used, expression);
    used = append_text(out, out_size, used, "\n        ");
    for (int i = 0; i < pos; i++) {
        used = append_text(out, out_size, used, " ");
    }
    used = append_text(out, out_size, used, "^\n");
}

static int calc_evaluate_impl(const char *expression,
                              CalcAngleMode mode,
                              CalcComplex ans_value,
                              int has_ans,
                              CalcComplex *result,
                              char *error_buffer,
                              size_t error_buffer_size) {
    if (expression == NULL || result == NULL) {
        return -1;
    }

    Parser p;
    memset(&p, 0, sizeof(p));
    p.ans_value = ans_value;
    p.has_ans = has_ans;
    p.mode = mode;
    lexer_init(&p.lx, expression);
    lexer_next(&p.lx);

    CalcComplex value = parse_expression(&p);

    if (!p.failed && p.lx.type != TOK_END) {
        parser_set_error(&p, p.lx.token_pos, "表达式在此处无法继续解析");
    }

    if (p.failed) {
        if (error_buffer != NULL && error_buffer_size > 0) {
            format_error(p.message, p.error_pos, expression,
                         error_buffer, error_buffer_size);
        }
        return -1;
    }

    if (!isfinite(value.re) || !isfinite(value.im)) {
        if (error_buffer != NULL && error_buffer_size > 0) {
            snprintf(error_buffer, error_buffer_size,
                     "计算结果不是有限数（NaN 或 Inf），请检查表达式\n");
        }
        return -1;
    }

    *result = value;
    return 0;
}

/* 复数求值 API（完整，支持 j 与复数结果） */
int calc_evaluate_complex(const char *expression,
                          CalcAngleMode mode,
                          CalcComplex ans,
                          int has_ans,
                          CalcComplex *result,
                          char *error_buffer,
                          size_t error_buffer_size) {
    return calc_evaluate_impl(expression, mode, ans, has_ans, result,
                              error_buffer, error_buffer_size);
}

int calc_evaluate(const char *expression,
                  double *result,
                  char *error_buffer,
                  size_t error_buffer_size) {
    CalcComplex c = { 0, 0 };
    int rc = calc_evaluate_impl(expression, CALC_MODE_RAD, zc(0.0), 0, &c,
                                error_buffer, error_buffer_size);
    if (rc == 0) *result = c.re;
    return rc;
}

int calc_evaluate_with_ans(const char *expression,
                           double ans,
                           int has_ans,
                           double *result,
                           char *error_buffer,
                           size_t error_buffer_size) {
    CalcComplex c = { 0, 0 };
    int rc = calc_evaluate_impl(expression, CALC_MODE_RAD, zc(ans), has_ans, &c,
                                error_buffer, error_buffer_size);
    if (rc == 0) *result = c.re;
    return rc;
}

int calc_evaluate_mode(const char *expression,
                       CalcAngleMode mode,
                       double ans,
                       int has_ans,
                       double *result,
                       char *error_buffer,
                       size_t error_buffer_size) {
    CalcComplex c = { 0, 0 };
    int rc = calc_evaluate_impl(expression, mode, zc(ans), has_ans, &c,
                                error_buffer, error_buffer_size);
    if (rc == 0) *result = c.re;
    return rc;
}
