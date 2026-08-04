#pragma once

/**
 * 计算表达式的值（支持 + - * / 和括号）
 * @param expr 字符串表达式，如 "2+3*4" 或 "(2+3)*4"
 * @return 计算结果
 */
double evaluate_expression(const char *expr);