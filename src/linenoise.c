#include "linenoise.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <unistd.h>
#include <termios.h>
#endif

#define MAX_HISTORY 100
static char *history[MAX_HISTORY];
static int history_count = 0;

static int get_char() {
#ifdef _WIN32
    return _getch();
#else
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
#endif
}

char* linenoise(const char *prompt) {
    static char buffer[4096];
    int len = 0;
    printf("%s", prompt);
    fflush(stdout);

    while (1) {
        int c = get_char();
        if (c == 13 || c == 10) { // Enter
            printf("\n");
            break;
        } else if (c == 127 || c == 8) { // Backspace
            if (len > 0) {
                len--;
                printf("\b \b");
                fflush(stdout);
            }
        } else if (c == 27 || c == 224) {
            // 完全忽略方向键等控制序列，防止乱码
#ifdef _WIN32
            if (c == 224) _getch(); // 吃掉方向键码
#else
            if (c == 27) {
                get_char(); // 吃掉 '['
                get_char(); // 吃掉字母
            }
#endif
        } else if (c >= 32 && c < 127) {
            if (len < 4095) {
                buffer[len++] = c;
                buffer[len] = '\0';
                printf("%c", c);
                fflush(stdout);
            }
        }
    }
    buffer[len] = '\0';
    if (len > 0 && history_count < MAX_HISTORY) {
        history[history_count++] = strdup(buffer);
    }
    return strdup(buffer);
}

void linenoiseFree(void *ptr) { free(ptr); }
int linenoiseHistoryAdd(const char *line) {
    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(line);
        return 1;
    }
    return 0;
}
int linenoiseHistorySetMaxLen(int len) { (void)len; return 0; }
int linenoiseHistorySave(const char *filename) { (void)filename; return 0; }
int linenoiseHistoryLoad(const char *filename) { (void)filename; return 0; }
void linenoiseClearScreen(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
void linenoiseSetMultiLine(int ml) { (void)ml; }
void linenoisePrintKeyCodes(void) { }