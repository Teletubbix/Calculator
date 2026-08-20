/*
 * Calculator 主程序 v3.0.0
 *
 * 交互模式：
 *   ./Calculator
 *   支持连续计算；按 Enter 计算，按 Esc 退出。
 *
 * 命令行模式：
 *   ./Calculator "1/3" "Ans+1"
 *   ./Calculator --precision 2 "1/3" "Ans+1"
 */

#include "calculator.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#define APP_NAME "Calculator"
#define APP_VERSION "3.0.0"
#define MAX_LINE 1024
#define PRECISION_AUTO (-1)

/* ------------------------------------------------------------------ */
/* 跨平台按键读取：用于实现“Esc 立即退出”的连续运行模式                  */
/* ------------------------------------------------------------------ */

#ifndef _WIN32
static struct termios g_original_termios;
static int g_termios_saved = 0;

static void console_restore(void) {
    if (g_termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_original_termios);
        g_termios_saved = 0;
    }
}

static int console_enter_raw(void) {
    if (!isatty(STDIN_FILENO)) {
        return 0; /* 输入重定向（例如测试脚本）时保持普通行读取 */
    }
    if (tcgetattr(STDIN_FILENO, &g_original_termios) != 0) {
        return -1;
    }
    struct termios raw = g_original_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        return -1;
    }
    g_termios_saved = 1;
    atexit(console_restore);
    return 1; /* 成功进入逐键模式 */
}

static int console_read_key(unsigned char *key) {
    ssize_t n = read(STDIN_FILENO, key, 1);
    return (int)n; /* 1=读到, 0=EOF, -1=错误 */
}
#else
static int console_enter_raw(void) { return 1; }
static int console_read_key(unsigned char *key) {
    int ch = _getch();
    if (ch == EOF) return 0;
    *key = (unsigned char)ch;
    return 1;
}
#endif

/* ------------------------------------------------------------------ */
/* 会话状态：Ans 记忆 + 显示精度                                        */
/* ------------------------------------------------------------------ */

typedef struct {
    double ans;         /* 上一次成功计算的结果（完整精度保存） */
    int has_ans;        /* 是否已经产生过结果 */
    int precision;      /* 显示小数位数；-1 表示自动格式 */
} Session;

static void session_init(Session *s) {
    s->ans = 0.0;
    s->has_ans = 0;
    s->precision = PRECISION_AUTO;
}

static void print_banner(const Session *s) {
    printf("==================================================\n");
    printf("  %s v%s —— C 语言科学计算器\n", APP_NAME, APP_VERSION);
    printf("==================================================\n");
    if (s->has_ans) {
        printf("  当前 Ans = %.15g\n", s->ans);
    }
    if (s->precision >= 0) {
        printf("  当前显示精度：保留 %d 位小数\n", s->precision);
    } else {
        printf("  当前显示精度：自动\n");
    }
}

static void print_help(void) {
    printf(
        "\n"
        " 连续运行：输入表达式后按 Enter 计算；按 Esc 随时退出。\n"
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
        " 常量：pi 或 π（圆周率）、e（自然常数）、Ans（上次结果）。\n"
        "\n"
        " 命令（输入后按 Enter）：\n"
        "   precision N      设置显示精度，例如 precision 2 表示保留 2 位小数\n"
        "   precision auto   恢复自动格式（%.15g）\n"
        "   help             显示本帮助\n"
        "   quit / exit / q  退出（也可直接按 Esc）\n"
        "\n"
        " 示例：\n"
        "   > e^2                 = 7.38905609893065\n"
        "   > sin(Ans)            = sin(e^2)      ← Ans 就是上一次结果\n"
        "   > precision 2\n"
        "   > 1/3                 = 0.33\n"
        "   > sqrt(9) + 5!        = 123\n"
        "   > cosd(60)            = 0.5\n"
        "\n"
        " 注意：\n"
        "   1. 暂不支持省略乘号，请写 2*pi 而不是 2pi。\n"
        "   2. 显示精度只影响屏幕显示，Ans 内部始终保存完整精度，\n"
        "      因此先 precision 2 再计算 1/3，Ans+1 仍按 1.333... 参与运算。\n"
        "\n");
}

/* 去掉一行首尾空白字符 */
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

static void format_result(double value, int precision, char *out, size_t out_size) {
    if (precision >= 0) {
        snprintf(out, out_size, "%.*f", precision, value);
    } else {
        snprintf(out, out_size, "%.15g", value);
    }
}

static int evaluate_line(Session *s, const char *line) {
    double result = 0.0;
    char error[512];

    if (calc_evaluate_with_ans(line, s->ans, s->has_ans,
                               &result, error, sizeof(error)) != 0) {
        fprintf(stderr, "[ERROR] %s", error);
        return -1;
    }

    s->ans = result;       /* 无论显示精度如何，Ans 始终保存完整精度 */
    s->has_ans = 1;

    char formatted[128];
    format_result(result, s->precision, formatted, sizeof(formatted));
    printf("= %s\n", formatted);
    return 0;
}

/*
 * 处理 precision 命令。
 * 返回：1 表示已处理（包括设置失败），0 表示不是该命令。
 */
static int handle_precision_command(Session *s, const char *line) {
    const char *p = line;
    if (strncmp(p, "precision", 9) == 0 &&
        (p[9] == '\0' || isspace((unsigned char)p[9]))) {
        p += 9;
    } else if (strncmp(p, "set", 3) == 0 &&
               isspace((unsigned char)p[3])) {
        p += 3;
        while (isspace((unsigned char)*p)) p++;
        if (strncmp(p, "precision", 9) != 0) {
            return 0;
        }
        p += 9;
    } else {
        return 0;
    }

    while (isspace((unsigned char)*p)) p++;
    if (strcmp(p, "auto") == 0) {
        s->precision = PRECISION_AUTO;
        printf("显示精度已恢复为自动格式。\n");
        return 1;
    }

    char *end = NULL;
    long value = strtol(p, &end, 10);
    while (isspace((unsigned char)*end)) end++;
    if (*p == '\0' || *end != '\0' || value < 0 || value > 15) {
        printf("用法：precision N（N 为 0~15），或 precision auto。\n");
        return 1;
    }
    s->precision = (int)value;
    printf("显示精度已设置为：保留 %d 位小数。\n", s->precision);
    return 1;
}

static int is_command(const char *line, const char *cmd) {
    return strcmp(line, cmd) == 0;
}

/* ------------------------------------------------------------------ */
/* 交互模式                                                             */
/* ------------------------------------------------------------------ */

static void print_prompt(void) {
    printf("> ");
    fflush(stdout);
}

/*
 * 逐键读取一行，支持 Backspace 删除；返回读取的字符数。
 * 返回 -1 表示用户按 Esc，返回 0 表示 EOF。
 */
static int read_line_raw(char *buf, size_t size) {
    size_t len = 0;
    buf[0] = '\0';

    while (1) {
        unsigned char key = 0;
        int n = console_read_key(&key);
        if (n <= 0) {
            return n;
        }

        /* Esc：立即退出连续计算模式 */
        if (key == 27) {
            return -1;
        }

        /* Enter */
        if (key == '\n' || key == '\r') {
            buf[len] = '\0';
            printf("\n");
            return (int)len;
        }

        /* Backspace */
        if (key == 127 || key == 8) {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        /* Ctrl+C / Ctrl+D */
        if (key == 3 || key == 4) {
            return 0;
        }

        /* 可打印字符：按字节存入（UTF-8 的 π 也能正常输入） */
        if (key >= 32 && key < 127 && len + 1 < size) {
            buf[len++] = (char)key;
            buf[len] = '\0';
            putchar(key);
            fflush(stdout);
        }
    }
}

/* 非交互输入（管道/重定向）时按整行读取，避免逐字节回显 */
static int read_line_buffered(char *buf, size_t size) {
    if (fgets(buf, (int)size, stdin) == NULL) {
        return 0;
    }
    return (int)strlen(buf);
}

static int run_repl(void) {
    Session session;
    session_init(&session);

    print_banner(&session);
    print_help();
    int raw_mode = console_enter_raw();

    while (1) {
        print_prompt();

        char line[MAX_LINE];
        int n = (raw_mode == 1) ? read_line_raw(line, sizeof(line))
                                : read_line_buffered(line, sizeof(line));
        if (n < 0) {
            printf("\n再见！\n");
            break;
        }
        if (n == 0) {
            printf("\n再见！\n");
            break;
        }

        char *input = trim_line(line);
        if (*input == '\0') {
            continue;
        }

        if (is_command(input, "quit") || is_command(input, "exit") ||
            is_command(input, "q")) {
            printf("再见！\n");
            break;
        }
        if (is_command(input, "help") || is_command(input, "h") ||
            is_command(input, "?")) {
            print_help();
            continue;
        }
        if (handle_precision_command(&session, input)) {
            continue;
        }

        evaluate_line(&session, input);
    }

    console_restore();
    return 0;
}

/* ------------------------------------------------------------------ */
/* 命令行模式                                                           */
/* ------------------------------------------------------------------ */

static int run_command_line(int argc, char **argv) {
    Session session;
    session_init(&session);
    int exit_code = 0;
    int next_is_precision = 0;

    for (int i = 1; i < argc; i++) {
        if (next_is_precision) {
            char *end = NULL;
            long value = strtol(argv[i], &end, 10);
            if (*argv[i] != '\0' && *end == '\0' && value >= 0 && value <= 15) {
                session.precision = (int)value;
            } else {
                fprintf(stderr, "[ERROR] --precision 后面的值必须是 0~15 的整数\n");
                return 1;
            }
            next_is_precision = 0;
            continue;
        }
        if (strcmp(argv[i], "--precision") == 0) {
            next_is_precision = 1;
            continue;
        }

        char *expr = trim_line(argv[i]);
        if (*expr == '\0') {
            continue;
        }
        printf("%s\n", expr);
        if (evaluate_line(&session, expr) != 0) {
            exit_code = 1;
        }
        printf("\n");
    }

    if (next_is_precision) {
        fprintf(stderr, "[ERROR] --precision 缺少参数\n");
        return 1;
    }
    return exit_code;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        return run_command_line(argc, argv);
    }
    return run_repl();
}
