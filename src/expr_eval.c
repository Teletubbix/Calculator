#include "expr_eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_STACK 100

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
        case '/': if (b==0) { fprintf(stderr,"\n[ERROR] 除数为0\n"); exit(1); } r = a/b; break;
    }
    push_num(s, r);
}

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

        // ---------- 处理字母（函数或常量） ----------
        if (isalpha(*p)) {
            char func[20] = {0};
            int i = 0;
            while (isalpha(*p) && i < 19) {
                func[i++] = *p;
                p++;
            }
            func[i] = '\0';

            // 如果后面跟着 '('，说明是函数（如 sin、cos）
            if (*p == '(') {
                p++; // 跳过 '('
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
                if (strcmp(func, "sin") == 0) result = sin(arg);
                else if (strcmp(func, "cos") == 0) result = cos(arg);
                else if (strcmp(func, "tan") == 0) result = tan(arg);
                else if (strcmp(func, "sqrt") == 0) result = sqrt(arg);
                else if (strcmp(func, "ln") == 0) result = log(arg);
                else if (strcmp(func, "log") == 0) result = log10(arg);
                else { fprintf(stderr, "[ERROR] 未知函数 '%s'\n", func); exit(1); }
                push_num(&s, result);
                continue;
            } 
            // 否则，说明是常量（π 或 e）
            else {
                double val = 0;
                if (strcmp(func, "π") == 0) {
                    val = 3.141592653589793;
                } else if (strcmp(func, "e") == 0) {
                    val = 2.718281828459045;
                } else {
                    fprintf(stderr, "[ERROR] 未知常量或函数 '%s'\n", func);
                    exit(1);
                }
                push_num(&s, val);
                continue;
            }
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
        fprintf(stderr, "[ERROR] 非法字符 '%c'\n", *p);
        exit(1);
    }
    while (!is_empty_op(&s)) compute_once(&s);
    return peek_num(&s);
}

double evaluate_expression(const char *expr) {
    return evaluate_core(expr, -1);
}