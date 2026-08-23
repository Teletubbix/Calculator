/*
 * Calculator — 版权所有 (C) 2026 Teletubbix (Yuanhang Jiang)
 * 本程序以 GNU Affero General Public License v3.0 传播；详见 LICENSE。
 */

/*
 * Calculator 主程序 v3.1.0
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
#include "units.h"
#include "matrix.h"
#include "complex.h"
#include "db.h"

#include <ctype.h>
#include <math.h>
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
#define APP_VERSION "4.6.0"
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
static void console_restore(void) { /* Windows 无需恢复终端设置 */ }
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
    CalcComplex ans;    /* 上一次成功计算的结果（复数，完整精度保存） */
    int has_ans;        /* 是否已经产生过结果 */
    int precision;      /* 显示小数位数；-1 表示自动格式 */
    CalcAngleMode mode; /* 三角函数角度制 */
    char hist[100][MAX_LINE];  /* 历史记录（过去的表达式） */
    CalcComplex hist_res[100]; /* 对应的结果 */
    int hist_count;
    int hist_pos;              /* 上/下箭头导航位置；-1 表示未在导航 */
} Session;

static void session_init(Session *s) {
    s->ans.re = 0.0;
    s->ans.im = 0.0;
    s->has_ans = 0;
    s->precision = PRECISION_AUTO;
    s->mode = CALC_MODE_RAD;
    s->hist_count = 0;
    s->hist_pos = -1;
}

static const char *mode_name(CalcAngleMode mode) {
    switch (mode) {
        case CALC_MODE_DEG:  return "角度制(DEG)";
        case CALC_MODE_GRAD: return "百分度(GRAD)";
        case CALC_MODE_RAD:
        default:             return "弧度制(RAD)";
    }
}

static void print_banner(const Session *s) {
    printf("==================================================\n");
    printf("  %s v%s —— C 语言科学计算器\n", APP_NAME, APP_VERSION);
    printf("  版权所有 (C) 2026 Teletubbix · 依 GNU AGPL-3.0 发布 (见 LICENSE)\n");
    printf("==================================================\n");
    if (s->has_ans) {
        double tol = 1e-12 * (1.0 + fabs(s->ans.re) + fabs(s->ans.im));
        char ab[96];
        if (fabs(s->ans.im) < tol) {
            snprintf(ab, sizeof(ab), "%.15g", s->ans.re);
        } else {
            snprintf(ab, sizeof(ab), "%.15g %s %.15gj", s->ans.re,
                     (s->ans.im < 0.0 ? "-" : "+"), fabs(s->ans.im));
        }
        printf("  当前 Ans = %s\n", ab);
    }
    if (s->precision >= 0) {
        printf("  当前显示精度：保留 %d 位小数\n", s->precision);
    } else {
        printf("  当前显示精度：自动\n");
    }
    printf("  当前角度制：%s\n", mode_name(s->mode));
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
        " 反三角：asin(x) acos(x) atan(x) / asind(x) acosd(x) atand(x)\n"
        " 双曲：sinh(x) cosh(x) tanh(x)\n"
        " 取整：floor(x) ceil(x) round(x) trunc(x)  其他：sign(x) atan2(y,x)\n"
        " 二元：mod(a,b) gcd(a,b) lcm(a,b) comb(n,k) perm(n,k) logn(x,base)\n"
        "   说明：ln 是自然对数，log 是以 10 为底的对数，logn(x,b) 是以 b 为底 x 的对数。\n"
        " 矩阵（方阵为主）：det2(a,b,c,d) trace2(a,b,c,d) 为 2x2，\n"
        "       det3(9个参数) trace3(9个参数) 为 3x3（按行优先排列元素）。\n"
        "\n"
        " 极坐标：r∠θ 或 r@θ（θ 按当前角度制）——相量记法，如 3∠30。\n"
        " 常量：pi 或 π（圆周率）、e（自然常数）、tau(2π)、phi(黄金分割)、Ans（上次结果）。\n"
        "\n"
        " 命令（输入后按 Enter）：\n"
        "   precision N      设置显示精度，例如 precision 2 表示保留 2 位小数\n"
        "   precision auto   恢复自动格式（%%）\n"
        "   mode deg/rad/grad 切换角度制（默认弧度制）\n"
        "   convert V from to 单位换算，例如 convert 1 km m、convert 0 dBm mW\n"
        "   help             显示本帮助\n"
        "   history           查看计算历史（上/下箭头亦可翻看）\n"
        "   clear             清空历史\n"
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

static void format_complex(CalcComplex z, int precision, char *out, size_t out_size) {
    double tol = 1e-12 * (1.0 + fabs(z.re) + fabs(z.im));
    if (fabs(z.im) < tol) {
        /* 实部即可 */
        if (precision >= 0) snprintf(out, out_size, "%.*f", precision, z.re);
        else                 snprintf(out, out_size, "%.15g", z.re);
        return;
    }
    char re[64] = {0}, im[64] = {0};
    if (precision >= 0) {
        snprintf(re, sizeof(re), "%.*f", precision, z.re);
        snprintf(im, sizeof(im), "%.*f", precision, fabs(z.im));
    } else {
        snprintf(re, sizeof(re), "%.15g", z.re);
        snprintf(im, sizeof(im), "%.15g", fabs(z.im));
    }
    snprintf(out, out_size, "%s %s %sj", re, (z.im < 0.0 ? "-" : "+"), im);
}

static int evaluate_line(Session *s, const char *line) {
    CalcComplex result = {0, 0};
    char error[512];

    if (calc_evaluate_complex(line, s->mode, s->ans, s->has_ans,
                              &result, error, sizeof(error)) != 0) {
        fprintf(stderr, "[ERROR] %s", error);
        return -1;
    }

    s->ans = result;       /* 无论显示精度如何，Ans 始终保存完整复数精度 */
    s->has_ans = 1;

    /* 记录到历史 */
    if (s->hist_count < 100) {
        strncpy(s->hist[s->hist_count], line, MAX_LINE - 1);
        s->hist[s->hist_count][MAX_LINE - 1] = '\0';
        s->hist_res[s->hist_count] = result;
        s->hist_count++;
        s->hist_pos = -1;
    }

    char formatted[160];
    format_complex(result, s->precision, formatted, sizeof(formatted));
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

/*
 * 处理 mode 命令。
 * 返回：1 表示已处理（包括设置失败），0 表示不是 mode 命令。
 */
static int handle_mode_command(Session *s, const char *line) {
    const char *p = line;
    if (strncmp(p, "mode", 4) != 0 ||
        (p[4] != '\0' && !isspace((unsigned char)p[4]))) {
        return 0;
    }
    p += 4;
    while (isspace((unsigned char)*p)) p++;

    if (strncmp(p, "deg", 3) == 0 && (p[3] == '\0' || isspace((unsigned char)p[3]))) {
        s->mode = CALC_MODE_DEG;
        printf("角度制已设置为：角度制(DEG)。sin/cos/tan 现在按角度解释。\n");
        return 1;
    }
    if (strncmp(p, "grad", 4) == 0 && (p[4] == '\0' || isspace((unsigned char)p[4]))) {
        s->mode = CALC_MODE_GRAD;
        printf("角度制已设置为：百分度(GRAD)。sin/cos/tan 现在按百分度解释。\n");
        return 1;
    }
    if (strncmp(p, "rad", 3) == 0 && (p[3] == '\0' || isspace((unsigned char)p[3]))) {
        s->mode = CALC_MODE_RAD;
        printf("角度制已设置为：弧度制(RAD)。\n");
        return 1;
    }
    printf("用法：mode deg（角度） / mode rad（弧度） / mode grad（百分度）。\n");
    return 1;
}

/*
 * 处理 convert 命令：convert <数值> <源单位> <目标单位>。
 * 返回：1 表示已处理，0 表示不是 convert 命令。
 */
static int handle_convert_command(Session *s, const char *line) {
    (void)s;
    const char *p = line;
    if (strncmp(p, "convert", 7) != 0 ||
        (p[7] != '\0' && !isspace((unsigned char)p[7]))) {
        return 0;
    }
    p += 7;
    while (isspace((unsigned char)*p)) p++;

    char from[32], to[32];
    double value = 0.0;
    if (sscanf(p, "%lf %31s %31s", &value, from, to) != 3) {
        printf("用法：convert <数值> <源单位> <目标单位>，例如 convert 1 km m\n");
        return 1;
    }
    double out = 0.0;
    char err[256] = {0};
    if (unit_convert(value, from, to, &out, err, sizeof(err)) != 0) {
        printf("[ERROR] %s\n", err);
    } else {
        printf("= %g %s\n", out, to);
    }
    return 1;
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

        /* Esc / 方向键：ESC [ A(上) / B(下)；Windows 是 224 + 'H'/'P' */
        if (key == 27) {
            unsigned char nxt = 0;
            if (console_read_key(&nxt) == 1 && nxt == '[') {
                unsigned char q = 0;
                if (console_read_key(&q) == 1) {
                    if (q == 'A') return -2;   /* 上 */
                    if (q == 'B') return -3;   /* 下 */
                }
                return -1;   /* 其它序列 → 退出 */
            }
            return -1;       /* 单独 Esc → 退出 */
        }
        if (key == 224) {    /* Windows 方向键前缀 */
            unsigned char q = 0;
            if (console_read_key(&q) == 1) {
                if (q == 'H') return -2;   /* 上 */
                if (q == 'P') return -3;   /* 下 */
            }
            continue;
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

        /* 可打印字符：按字节存入（key >= 32 即可，允许 UTF-8 多字节字符如 π 正常输入） */
        if (key >= 32 && len + 1 < size) {
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

    print_prompt();
    while (1) {
        char line[MAX_LINE];
        int n = (raw_mode == 1) ? read_line_raw(line, sizeof(line))
                                : read_line_buffered(line, sizeof(line));

        /* 上箭头：历史上一条 */
        if (n == -2) {
            if (raw_mode == 1 && session.hist_count > 0) {
                if (session.hist_pos < 0) session.hist_pos = session.hist_count - 1;
                else if (session.hist_pos > 0) session.hist_pos--;
                strncpy(line, session.hist[session.hist_pos], MAX_LINE - 1);
                line[MAX_LINE - 1] = '\0';
                printf("\r\033[K> %s", line);
                fflush(stdout);
            }
            continue;
        }
        /* 下箭头：返回下一条 */
        if (n == -3) {
            if (raw_mode == 1 && session.hist_pos >= 0) {
                session.hist_pos--;
                if (session.hist_pos < 0) {
                    line[0] = '\0';
                    printf("\r\033[K> ");
                } else {
                    strncpy(line, session.hist[session.hist_pos], MAX_LINE - 1);
                    line[MAX_LINE - 1] = '\0';
                    printf("\r\033[K> %s", line);
                }
                fflush(stdout);
            }
            continue;
        }
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
            print_prompt();
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
            print_prompt();
            continue;
        }
        if (is_command(input, "history") || is_command(input, "hist")) {
            if (session.hist_count == 0) {
                printf("  (暂无历史)\n");
            } else {
                for (int i = 0; i < session.hist_count; i++) {
                    char r[96];
                    format_complex(session.hist_res[i], session.precision, r, sizeof(r));
                    printf("  %2d. %s = %s\n", i + 1, session.hist[i], r);
                }
            }
            print_prompt();
            continue;
        }
        if (is_command(input, "clear") || is_command(input, "cls")) {
            session.hist_count = 0;
            session.hist_pos = -1;
            printf("历史已清空。\n");
            print_prompt();
            continue;
        }
        if (handle_precision_command(&session, input)) {
            print_prompt();
            continue;
        }
        if (handle_mode_command(&session, input)) {
            print_prompt();
            continue;
        }
        if (handle_convert_command(&session, input)) {
            print_prompt();
            continue;
        }

        evaluate_line(&session, input);
        print_prompt();
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
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-V") == 0) {
            printf("%s v%s\n", APP_NAME, APP_VERSION);
            return 0;
        }
        if (strcmp(argv[i], "--mode") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "[ERROR] --mode 需要一个参数 deg/rad/grad\n");
                return 1;
            }
            const char *m = argv[++i];
            if (strcmp(m, "deg") == 0)     session.mode = CALC_MODE_DEG;
            else if (strcmp(m, "grad") == 0) session.mode = CALC_MODE_GRAD;
            else if (strcmp(m, "rad") == 0) session.mode = CALC_MODE_RAD;
            else {
                fprintf(stderr, "[ERROR] --mode 只能是 deg/rad/grad\n");
                return 1;
            }
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

/* 在启动时把各算法库(矩阵/复数)注册进引擎，供表达式调用 */
static void register_algorithm_functions(void) {
    size_t n;
    const calc_function *f = calc_matrix_functions(&n);
    calc_register_functions(f, n);
    f = calc_complex_functions(&n);
    calc_register_functions(f, n);
    f = calc_db_functions(&n);
    calc_register_functions(f, n);
}

int main(int argc, char **argv) {
    register_algorithm_functions();
    if (argc > 1) {
        return run_command_line(argc, argv);
    }
    return run_repl();
}
