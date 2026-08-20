/*
 * Calculator 主程序
 *
 * 用法一：交互模式
 *   ./Calculator
 *   然后逐行输入表达式，例如：
 *     > (1+2)*3^2
 *     > sqrt(9) + 5!
 *     > sin(pi/2)
 *   help 查看帮助，quit / exit 退出。
 *
 * 用法二：命令行直接计算
 *   ./Calculator "2^10" "log(1000)"
 */

#include "calculator.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define APP_NAME "Calculator"
#define APP_VERSION "2.0.0"
#define MAX_LINE 1024

static void print_banner(void) {
    printf("==================================================\n");
    printf("  %s v%s —— C 语言科学计算器\n", APP_NAME, APP_VERSION);
    printf("==================================================\n");
}

static void print_help(void) {
    printf(
        "\n"
        "支持直接输入数学表达式，按回车计算。\n"
        "\n"
        " 运算符：\n"
        "   + - * /      加、减、乘、除\n"
        "   ^            乘方（右结合，例如 2^3^2 = 2^(3^2)）\n"
        "   !            阶乘（后缀，例如 5! = 120）\n"
        "\n"
        " 函数（输入弧度）：sin(x) cos(x) tan(x)\n"
        " 函数（输入角度）：sind(x) cosd(x) tand(x)\n"
        " 其他函数：sqrt(x) ln(x) log(x) log2(x) pow(a,b) exp(x) abs(x)\n"
        "   说明：ln 是自然对数，log 是以 10 为底的对数。\n"
        "\n"
        " 常量：pi 或 π（圆周率）、e（自然常数），不区分大小写。\n"
        "\n"
        " 示例：\n"
        "   2 + 3 * 4          = 14\n"
        "   (2 + 3) * 4        = 20\n"
        "   2^10               = 1024\n"
        "   sqrt(9) + 5!       = 123\n"
        "   sin(pi/2)          = 1\n"
        "   cosd(60)           = 0.5（角度制）\n"
        "   ln(e^3)            = 3\n"
        "   log(1000)          = 3\n"
        "   2 * pi             = 6.283185307179586\n"
        "\n"
        " 注意：暂不支持省略乘号，请写 2*pi 而不是 2pi。\n"
        "       输入 help 查看本帮助，quit 或 exit 退出。\n"
        "\n");
}

/* 去掉一行首尾的空白字符（含换行符） */
static char *trim_line(char *s) {
    while (isspace((unsigned char)*s)) {
        s++;
    }
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[--len] = '\0';
    }
    return s;
}

static int evaluate_line(const char *line) {
    double result = 0.0;
    char error[512];

    if (calc_evaluate(line, &result, error, sizeof(error)) != 0) {
        fprintf(stderr, "[ERROR] %s", error);
        return -1;
    }

    /* %g 会自动去掉多余的 0，整数结果不显示小数点 */
    printf("= %.15g\n", result);
    return 0;
}

static int is_command(const char *line, const char *cmd) {
    return strcmp(line, cmd) == 0;
}

/* 交互模式：反复读取一行并求值，直到 EOF 或退出命令 */
static int run_repl(void) {
    char line[MAX_LINE];

    print_banner();
    print_help();

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n再见！\n");
            return 0;
        }

        char *input = trim_line(line);
        if (*input == '\0') {
            continue;
        }

        if (is_command(input, "quit") || is_command(input, "exit") ||
            is_command(input, "q")) {
            printf("再见！\n");
            return 0;
        }
        if (is_command(input, "help") || is_command(input, "h") ||
            is_command(input, "?")) {
            print_help();
            continue;
        }

        evaluate_line(input);
    }
}

/* 命令行模式：./Calculator "表达式1" "表达式2" ... */
static int run_command_line(int argc, char **argv) {
    int exit_code = 0;

    for (int i = 1; i < argc; i++) {
        char *expr = trim_line(argv[i]);
        if (*expr == '\0') {
            continue;
        }
        printf("%s\n", expr);
        if (evaluate_line(expr) != 0) {
            exit_code = 1;
        }
        printf("\n");
    }
    return exit_code;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        return run_command_line(argc, argv);
    }
    return run_repl();
}
