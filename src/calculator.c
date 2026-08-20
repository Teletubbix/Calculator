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
    double ans_value;         /* 上一次计算结果，供 Ans 使用 */
    int has_ans;              /* 是否已经有上一次计算结果 */
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

static double parse_expression(Parser *p);
static double parse_term(Parser *p);
static double parse_unary(Parser *p);
static double parse_power(Parser *p);
static double parse_postfix(Parser *p);
static double parse_primary(Parser *p);

/* 二元运算，同时做定义域检查 */
static double binary_op(Parser *p, int pos, char op, double a, double b) {
    switch (op) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            if (b == 0.0) {
                parser_set_error(p, pos, "除法错误：除数不能为 0");
                return 0.0;
            }
            return a / b;
        case '^': {
            errno = 0;
            double r = pow(a, b);
            if (errno == EDOM || isnan(r)) {
                parser_set_error(p, pos,
                                 "乘方错误：底数为负数时，指数必须是整数（例如 (-2)^2，"
                                 "而 (-2)^0.5 在实数范围内无定义）");
                return 0.0;
            }
            if (isinf(r) && isfinite(a) && isfinite(b)) {
                parser_set_error(p, pos, "乘方结果超出 double 可表示的范围");
                return 0.0;
            }
            return r;
        }
        default:
            parser_set_error(p, pos, "内部错误：未知运算符 '%c'", op);
            return 0.0;
    }
}

/* 一元函数，同时做定义域检查 */
static double unary_function(Parser *p, int pos, const char *name, double x) {
    if (ident_equals(name, "sin")) {
        return sin(x);
    }
    if (ident_equals(name, "cos")) {
        return cos(x);
    }
    if (ident_equals(name, "tan")) {
        return tan(x);
    }
    if (ident_equals(name, "sind")) {
        return sin(x * M_PI / 180.0);   /* 输入是角度制 */
    }
    if (ident_equals(name, "cosd")) {
        return cos(x * M_PI / 180.0);
    }
    if (ident_equals(name, "tand")) {
        return tan(x * M_PI / 180.0);
    }
    if (ident_equals(name, "sqrt")) {
        if (x < 0.0) {
            parser_set_error(p, pos, "sqrt 定义域错误：负数没有实数平方根");
            return 0.0;
        }
        return sqrt(x);
    }
    if (ident_equals(name, "ln")) {
        if (x <= 0.0) {
            parser_set_error(p, pos, "ln 定义域错误：真数必须大于 0");
            return 0.0;
        }
        return log(x);
    }
    if (ident_equals(name, "log")) {    /* 常用对数，底数为 10 */
        if (x <= 0.0) {
            parser_set_error(p, pos, "log 定义域错误：真数必须大于 0");
            return 0.0;
        }
        return log10(x);
    }
    if (ident_equals(name, "log2")) {   /* 底数为 2 的对数 */
        if (x <= 0.0) {
            parser_set_error(p, pos, "log2 定义域错误：真数必须大于 0");
            return 0.0;
        }
        return log2(x);
    }
    if (ident_equals(name, "exp")) {
        errno = 0;
        double r = exp(x);
        if (errno == ERANGE) {
            parser_set_error(p, pos, "exp 结果超出 double 可表示的范围");
            return 0.0;
        }
        return r;
    }
    if (ident_equals(name, "abs")) {
        return fabs(x);
    }
    parser_set_error(p, pos, "未知函数 '%s'", name);
    return 0.0;
}

/* 阶乘只对 0~170 的非负整数有定义（171! 已超出 double 范围） */
static double factorial_op(Parser *p, int pos, double x) {
    if (x < 0.0 || floor(x) != x) {
        parser_set_error(p, pos, "阶乘错误：n! 要求 n 是非负整数（例如 5!）");
        return 0.0;
    }
    if (x > 170.0) {
        parser_set_error(p, pos, "阶乘错误：n 太大，171! 已超出 double 可表示的范围");
        return 0.0;
    }
    double r = 1.0;
    for (int i = 2; i <= (int)x; i++) {
        r *= (double)i;
    }
    return r;
}

/*
 * expression -> term (('+' | '-') term)*
 */
static double parse_expression(Parser *p) {
    double left = parse_term(p);
    if (p->failed) {
        return 0.0;
    }
    while (p->lx.type == TOK_PLUS || p->lx.type == TOK_MINUS) {
        char op = (p->lx.type == TOK_PLUS) ? '+' : '-';
        int op_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        double right = parse_term(p);
        if (p->failed) {
            return 0.0;
        }
        left = binary_op(p, op_pos, op, left, right);
        if (p->failed) {
            return 0.0;
        }
    }
    return left;
}

/*
 * term -> unary (('*' | '/') unary)*
 */
static double parse_term(Parser *p) {
    double left = parse_unary(p);
    if (p->failed) {
        return 0.0;
    }
    while (p->lx.type == TOK_STAR || p->lx.type == TOK_SLASH) {
        char op = (p->lx.type == TOK_STAR) ? '*' : '/';
        int op_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        double right = parse_unary(p);
        if (p->failed) {
            return 0.0;
        }
        left = binary_op(p, op_pos, op, left, right);
        if (p->failed) {
            return 0.0;
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
static double parse_unary(Parser *p) {
    if (p->lx.type == TOK_PLUS || p->lx.type == TOK_MINUS) {
        char sign = (p->lx.type == TOK_PLUS) ? '+' : '-';
        lexer_next(&p->lx);
        double value = parse_unary(p);
        if (p->failed) {
            return 0.0;
        }
        if (sign == '-') {
            return -value;
        }
        return value;
    }
    return parse_power(p);
}

/*
 * power -> postfix ('^' unary)?
 *
 * 幂运算右结合：2^3^2 = 2^(3^2) = 512。
 * 指数部分复用 unary，因此允许 2^-2 这类写法。
 */
static double parse_power(Parser *p) {
    double base = parse_postfix(p);
    if (p->failed) {
        return 0.0;
    }
    if (p->lx.type == TOK_CARET) {
        int op_pos = p->lx.token_pos;
        lexer_next(&p->lx);
        double exponent = parse_unary(p);
        if (p->failed) {
            return 0.0;
        }
        return binary_op(p, op_pos, '^', base, exponent);
    }
    return base;
}

/*
 * postfix -> primary '!'?
 */
static double parse_postfix(Parser *p) {
    double value = parse_primary(p);
    if (p->failed) {
        return 0.0;
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
static double parse_primary(Parser *p) {
    if (p->lx.type == TOK_NUMBER) {
        double value = p->lx.number;
        lexer_next(&p->lx);
        return value;
    }

    if (p->lx.type == TOK_IDENT) {
        char name[MAX_IDENT_LEN + 1];
        strncpy(name, p->lx.ident, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        int ident_pos = p->lx.token_pos;
        lexer_next(&p->lx);

        if (p->lx.type == TOK_LPAREN) {
            lexer_next(&p->lx); /* 跳过 '(' */

            if (ident_equals(name, "pow")) {
                /* pow(a, b)：两个参数 */
                double base = parse_expression(p);
                if (p->failed) {
                    return 0.0;
                }
                if (p->lx.type != TOK_COMMA) {
                    parser_set_error(p, p->lx.token_pos,
                                     "pow 需要两个参数，格式：pow(底数, 指数)");
                    return 0.0;
                }
                lexer_next(&p->lx);
                double exponent = parse_expression(p);
                if (p->failed) {
                    return 0.0;
                }
                if (p->lx.type != TOK_RPAREN) {
                    parser_set_error(p, p->lx.token_pos, "pow 缺少右括号 ')'");
                    return 0.0;
                }
                lexer_next(&p->lx);
                return binary_op(p, ident_pos, '^', base, exponent);
            }

            double arg = parse_expression(p);
            if (p->failed) {
                return 0.0;
            }
            if (p->lx.type != TOK_RPAREN) {
                parser_set_error(p, p->lx.token_pos, "函数 '%s' 缺少右括号 ')'", name);
                return 0.0;
            }
            lexer_next(&p->lx);
            return unary_function(p, ident_pos, name, arg);
        }

        /* 不是函数，就按常量处理 */
        if (ident_equals(name, "pi")) {
            return M_PI;
        }
        if (ident_equals(name, "e")) {
            return M_E;
        }
        if (ident_equals(name, "ans")) {
            if (!p->has_ans) {
                parser_set_error(p, ident_pos,
                                 "还没有上一次计算结果，请先完成一次计算再使用 Ans");
                return 0.0;
            }
            return p->ans_value;
        }
        parser_set_error(p, ident_pos, "未知常量 '%s'（可用常量：pi、e、Ans）", name);
        return 0.0;
    }

    if (p->lx.type == TOK_LPAREN) {
        lexer_next(&p->lx);
        double value = parse_expression(p);
        if (p->failed) {
            return 0.0;
        }
        if (p->lx.type != TOK_RPAREN) {
            parser_set_error(p, p->lx.token_pos, "缺少右括号 ')'");
            return 0.0;
        }
        lexer_next(&p->lx);
        return value;
    }

    if (p->lx.type == TOK_ERROR) {
        parser_set_error(p, p->lx.token_pos, "存在无法识别的字符或数字格式");
        return 0.0;
    }

    parser_set_error(p, p->lx.token_pos, "此处应为数字、常量、函数或左括号 '('");
    return 0.0;
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
                             double ans_value,
                             int has_ans,
                             double *result,
                             char *error_buffer,
                             size_t error_buffer_size) {
    if (expression == NULL || result == NULL) {
        return -1;
    }

    Parser p;
    memset(&p, 0, sizeof(p));
    p.ans_value = ans_value;
    p.has_ans = has_ans;
    lexer_init(&p.lx, expression);
    lexer_next(&p.lx);

    double value = parse_expression(&p);

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

    if (isnan(value) || isinf(value)) {
        if (error_buffer != NULL && error_buffer_size > 0) {
            snprintf(error_buffer, error_buffer_size,
                     "计算结果不是有限实数（NaN 或 Inf），请检查表达式\n");
        }
        return -1;
    }

    *result = value;
    return 0;
}

int calc_evaluate(const char *expression,
                  double *result,
                  char *error_buffer,
                  size_t error_buffer_size) {
    return calc_evaluate_impl(expression, 0.0, 0, result,
                              error_buffer, error_buffer_size);
}

int calc_evaluate_with_ans(const char *expression,
                           double ans,
                           int has_ans,
                           double *result,
                           char *error_buffer,
                           size_t error_buffer_size) {
    return calc_evaluate_impl(expression, ans, has_ans, result,
                              error_buffer, error_buffer_size);
}
