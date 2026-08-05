#include "expr_eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_STACK 100

// ------------------------------------------------------------
// 错误报告辅助函数（打印带箭头的错误位置）
// ------------------------------------------------------------
static void report_error(const char *expr, int pos, const char *msg) {
    fprintf(stderr, "\n[ERROR] %s\n", msg);
    fprintf(stderr, "        %s\n", expr);
    fprintf(stderr, "        ");
    for (int i = 0; i < pos; i++) fprintf(stderr, " ");
    fprintf(stderr, "^\n\n");
}

// ------------------------------------------------------------
// 预扫描：检查括号匹配和非法字符
// ------------------------------------------------------------
static int prescan_expression(const char *expr) {
    int len = strlen(expr);
    int paren_count = 0;

    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)expr[i];

        // 检查中文左括号 '（' (UTF-8: EF BC 88)
        if (c == 0xEF && expr[i+1] == 0xBC && expr[i+2] == 0x88) {
            report_error(expr, i, "检测到中文左括号 '（'，请使用英文括号 '('");
            return 0;
        }
        // 检查中文右括号 '）' (UTF-8: EF BC 89)
        if (c == 0xEF && expr[i+1] == 0xBC && expr[i+2] == 0x89) {
            report_error(expr, i, "检测到中文右括号 '）'，请使用英文括号 ')'");
            return 0;
        }

        // 检查英文括号匹配
        if (expr[i] == '(') paren_count++;
        else if (expr[i] == ')') {
            paren_count--;
            if (paren_count < 0) {
                report_error(expr, i, "右括号 ')' 多余，没有匹配的左括号");
                return 0;
            }
        }

        // 检查非法字符（只允许数字、+-*/、.、空格、字母、括号）
        if (!isdigit(expr[i]) && !isalpha(expr[i]) && 
            !strchr("+-*/.() ", expr[i])) {
            // 检查是否是 π（UTF-8 编码允许）
            if (!(c == 0xCF && expr[i+1] == 0x80)) {
                report_error(expr, i, "存在非法字符，仅支持数字、字母、+ - * / . ( ) 和空格");
                return 0;
            }
        }
    }

    if (paren_count != 0) {
        int pos = (paren_count > 0) ? len : 0;
        report_error(expr, pos, "括号不匹配，请检查左括号是否都有对应的右括号");
        return 0;
    }

    return 1; // 扫描通过
}

// ------------------------------------------------------------
// 栈结构及操作（与之前完全一致）
// ------------------------------------------------------------
typedef struct {
    double num_stack[MAX_STACK];
    int num_top;
    char op_stack[MAX_STACK];
    int op_top;
} Stack;

static void init_stack(Stack *s) { s->num_top = -1; s->op_top = -1; }
static void push_num(Stack *s, double v) { if (s->num_top < MAX_STACK-1) s->num_stack[++s->num_top] = v; }
static double pop_num(Stack *s) { if (s->num_top >= 0) return s->num_stack[s->num_top--]; return 0; }
static double peek_num(Stack *s) { return s->num_stack[s->num_top]; }
static void push_op(Stack *s, char c) { if (s->op_top < MAX_STACK-1) s->op_stack[++s->op_top] = c; }
static char pop_op(Stack *s) { if (s->op_top >= 0) return s->op_stack[s->op_top--]; return '\0'; }
static char peek_op(Stack *s) { return s->op_stack[s->op_top]; }
static int is_empty_op(Stack *s) { return s->op_top < 0; }

static int priority(char c) {
    if (c == '+' || c == '-') return 1;
    if (c == '*' || c == '/') return 2;
    return 0;
}

static void compute_once(Stack *s) {
    if (s->num_top < 1 || s->op_top < 0) return;
    char op = pop_op(s);
    double b = pop_num(s);
    double a = pop_num(s);
    double r = 0;
    switch(op) {
        case '+': r = a+b; break;
        case '-': r = a-b; break;
        case '*': r = a*b; break;
        case '/': 
            if (b == 0) { 
                fprintf(stderr, "\n[ERROR] 计算过程中发现除数为 0，表达式未定义！\n\n");
                exit(EXIT_FAILURE); 
            } 
            r = a/b; 
            break;
    }
    push_num(s, r);
}

// ------------------------------------------------------------
// 核心递归求值（与之前一致，但使用更安全的错误退出）
// ------------------------------------------------------------
static double evaluate_core(const char *expr, int len) {
    Stack s;
    init_stack(&s);
    const char *p = expr;
    const char *end = (len >= 0) ? expr + len : NULL;
    while (*p && (end == NULL || p < end)) {
        if (isspace(*p)) { p++; continue; }

        if (isdigit(*p) || *p == '.') {
            char *endptr;
            double val = strtod(p, &endptr);
            push_num(&s, val);
            p = endptr;
            continue;
        }

        if (isalpha(*p)) {
            char word[20] = {0};
            int i = 0;
            while (isalpha(*p) && i < 19) { word[i++] = *p; p++; }
            word[i] = '\0';

            if (*p == '(') {
                p++;
                const char *sub_start = p;
                int paren_count = 1;
                while (*p && paren_count > 0) {
                    if (*p == '(') paren_count++;
                    else if (*p == ')') paren_count--;
                    p++;
                }
                int sub_len = (p - sub_start) - 1;
                char sub_expr[256] = {0};
                if (sub_len > 0 && sub_len < 255) {
                    strncpy(sub_expr, sub_start, sub_len);
                    sub_expr[sub_len] = '\0';
                }
                double arg = evaluate_core(sub_expr, -1);
                double result = 0;
                if (strcmp(word, "sin") == 0) result = sin(arg);
                else if (strcmp(word, "cos") == 0) result = cos(arg);
                else if (strcmp(word, "tan") == 0) result = tan(arg);
                else if (strcmp(word, "sqrt") == 0) result = sqrt(arg);
                else if (strcmp(word, "ln") == 0) result = log(arg);
                else if (strcmp(word, "log") == 0) result = log10(arg);
                else { 
                    fprintf(stderr, "[ERROR] 未知函数 '%s'\n", word); 
                    exit(EXIT_FAILURE); 
                }
                push_num(&s, result);
                continue;
            } else {
                double val = 0;
                if (strcmp(word, "pi") == 0) val = 3.141592653589793;
                else if (strcmp(word, "e") == 0) val = 2.718281828459045;
                else {
                    fprintf(stderr, "[ERROR] 未知常量或函数 '%s'\n", word);
                    exit(EXIT_FAILURE);
                }
                push_num(&s, val);
                continue;
            }
        }

        // 处理希腊字母 π
        if ((unsigned char)*p == 0xCF && *(p+1) == 0x80) {
            push_num(&s, 3.141592653589793);
            p += 2;
            continue;
        }

        if (*p == '(') { push_op(&s, '('); p++; continue; }
        if (*p == ')') {
            while (!is_empty_op(&s) && peek_op(&s) != '(') compute_once(&s);
            if (!is_empty_op(&s) && peek_op(&s) == '(') pop_op(&s);
            p++; continue;
        }
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            while (!is_empty_op(&s) && priority(peek_op(&s)) >= priority(*p)) compute_once(&s);
            push_op(&s, *p);
            p++; continue;
        }
        // 理论上预扫描已经过滤了所有非法字符，但为了安全保留
        fprintf(stderr, "[ERROR] 发现意外字符 '%c'\n", *p);
        exit(EXIT_FAILURE);
    }
    while (!is_empty_op(&s)) compute_once(&s);
    return peek_num(&s);
}

// ------------------------------------------------------------
// 对外接口：先预扫描，再求值
// ------------------------------------------------------------
double evaluate_expression(const char *expr) {
    // 1. 预扫描，如果不通过则直接返回 0
    if (!prescan_expression(expr)) {
        return 0.0;
    }
    // 2. 扫描通过，执行求值
    return evaluate_core(expr, -1);
}