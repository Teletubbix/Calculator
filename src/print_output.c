#include "print_output.h"
#include <stdio.h>

void print_result(double a, char op, double b, double result) {
    printf("%.2f %c %.2f = %.2f\n", a, op, b, result);
}