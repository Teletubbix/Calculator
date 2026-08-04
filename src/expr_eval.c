#include "expr_eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_STACK 100
static double num_stack[MAX_STACK];
static int num_top = -1;
static char op_stack[MAX_STACK];
static int op_top = -1;

static void push_num(double v) { if (num_top < MAX_STACK-1) num_stack[++num_top] = v; }
static double pop_num() { if (num_top >= 0) return num_stack[num_top--]; return 0; }
static double peek_num() { return num_stack[num_top]; }
static void push_op(char c) { if (op_top < MAX_STACK-1) op_stack[++op_top] = c; }
static char pop_op() { if (op_top >= 0) return op_stack[op_top--]; return '\0'; }
static char peek_op() { return op_stack[op_top]; }

static int priority(char c) {
    if (c == '+' || c == '-') return 1;
    if (c == '*' || c == '/') return 2;
    return 0;
}

static void compute_once() {
    if (num_top < 1 || op_top < 0) return;
    char op = pop_op();
    double b = pop_num();
    double a = pop_num();
    double r = 0;
    switch(op) {
        case '+': r = a+b; break;
        case '-': r = a-b; break;
        case '*': r = a*b; break;
        case '/': if (b==0) { fprintf(stderr,"除0错误\n"); exit(1); } r = a/b; break;
    }
    push_num(r);
}

double evaluate_expression(const char *expr) {
    num_top = -1; op_top = -1;
    const char *p = expr;
    while (*p) {
        if (isspace(*p)) { p++; continue; }
        if (isdigit(*p) || *p == '.') {
            char *end;
            double v = strtod(p, &end);
            push_num(v);
            p = end;
            continue;
        }
        if (*p == '(') { push_op('('); p++; continue; }
        if (*p == ')') {
            while (op_top >= 0 && peek_op() != '(') compute_once();
            if (peek_op() == '(') pop_op();
            p++; continue;
        }
        if (*p == '+' || *p == '-' || *p == '*' || *p == '/') {
            while (op_top >= 0 && priority(peek_op()) >= priority(*p)) compute_once();
            push_op(*p);
            p++; continue;
        }
        fprintf(stderr, "非法字符 %c\n", *p);
        exit(1);
    }
    while (op_top >= 0) compute_once();
    return peek_num();
}