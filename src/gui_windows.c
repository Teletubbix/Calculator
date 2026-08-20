/*
 * Calculator v3.0.0 —— Windows Win32 图形界面
 *
 * 界面说明：
 *   - 顶部是表达式输入框，可直接用键盘输入
 *   - 下面是结果标签
 *   - 数字、符号、运算符和常用函数都有独立按钮
 *   - Enter 计算，Esc 退出
 *   - "精度"按钮设置显示的小数位数
 *   - Ans 保存上一次计算结果
 *
 * 编译（MinGW）：
 *   x86_64-w64-mingw32-gcc -std=c11 -municode -mwindows \
 *       -Iinclude src/gui_windows.c src/calculator.c -o Calculator.exe -lm
 */

#include "calculator.h"

#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>

#define GUI_VERSION L"3.0.0"
#define PRECISION_AUTO (-1)

/* 控件 ID */
#define IDC_EDIT    1001
#define IDC_RESULT  1002
#define IDB_BASE    2000
#define IDB_EQUALS  (IDB_BASE + 0)
#define IDB_CLEAR   (IDB_BASE + 1)
#define IDB_BACK    (IDB_BASE + 2)
#define IDB_PREC    (IDB_BASE + 3)
#define IDB_FIRST_TOKEN (IDB_BASE + 10)

typedef struct {
    HWND hWnd;
    HWND hEdit;
    HWND hResult;
    double ans;        /* 上一次结果，完整精度 */
    int has_ans;       /* 是否已有 Ans */
    int precision;     /* -1 自动，0~15 保留小数 */
} AppState;

static HINSTANCE g_hInst;
static AppState g_app;
static WNDPROC g_oldEditProc;

/* 精度弹窗的临时状态 */
static HWND g_prec_dialog = NULL;
static HWND g_prec_edit = NULL;
static int g_prec_value = -2;
static BOOL g_prec_cancelled = FALSE;

/* ------------------------------------------------------------------ */
/* 工具函数                                                             */
/* ------------------------------------------------------------------ */

static void WideFromUtf8(const char *utf8, wchar_t *wide, int wide_size) {
    if (utf8 == NULL || wide == NULL || wide_size <= 0) return;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, wide_size);
    wide[wide_size - 1] = L'\0';
}

static void FormatResult(double value, int precision, wchar_t *out, int out_size) {
    if (precision >= 0) {
        _snwprintf(out, out_size, L"%.*f", precision, value);
    } else {
        _snwprintf(out, out_size, L"%.15g", value);
    }
    out[out_size - 1] = L'\0';
}

/* ------------------------------------------------------------------ */
/* 核心操作                                                             */
/* ------------------------------------------------------------------ */

static void AppendToken(const wchar_t *token) {
    SendMessageW(g_app.hEdit, EM_REPLACESEL, TRUE, (LPARAM)token);
    SetFocus(g_app.hEdit);
}

static void ClearAll(void) {
    SetWindowTextW(g_app.hEdit, L"");
    SetWindowTextW(g_app.hResult, L"");
    SetFocus(g_app.hEdit);
}

static void Backspace(void) {
    wchar_t buf[1024];
    GetWindowTextW(g_app.hEdit, buf, 1024);
    size_t len = wcslen(buf);
    if (len > 0) {
        buf[len - 1] = L'\0';
        SetWindowTextW(g_app.hEdit, buf);
        SendMessageW(g_app.hEdit, EM_SETSEL, (WPARAM)len - 1, (LPARAM)len - 1);
    }
    SetFocus(g_app.hEdit);
}

static void EvaluateExpression(void) {
    wchar_t wbuf[1024];
    GetWindowTextW(g_app.hEdit, wbuf, 1024);

    if (wcslen(wbuf) == 0) {
        SetWindowTextW(g_app.hResult, L"请先输入表达式");
        return;
    }

    char expr[2048];
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, expr, sizeof(expr), NULL, NULL);
    expr[sizeof(expr) - 1] = '\0';

    double result = 0.0;
    char error[512];
    if (calc_evaluate_with_ans(expr, g_app.ans, g_app.has_ans,
                               &result, error, sizeof(error)) != 0) {
        wchar_t werror[1024];
        WideFromUtf8(error, werror, 1024);
        MessageBoxW(g_app.hWnd, werror, L"输入有误", MB_OK | MB_ICONERROR);
        SetWindowTextW(g_app.hResult, L"表达式有误");
        return;
    }

    g_app.ans = result;
    g_app.has_ans = 1;

    wchar_t formatted[128];
    wchar_t label[160];
    FormatResult(result, g_app.precision, formatted, 128);
    _snwprintf(label, 160, L"= %ls", formatted);
    label[159] = L'\0';
    SetWindowTextW(g_app.hResult, label);
}

/* ------------------------------------------------------------------ */
/* 精度设置弹窗                                                         */
/* ------------------------------------------------------------------ */

LRESULT CALLBACK PrecisionProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    switch (msg) {
        case WM_CREATE: {
            CreateWindowExW(0, L"STATIC",
                            L"结果保留几位小数？输入 0~15 的整数。",
                            WS_CHILD | WS_VISIBLE,
                            15, 12, 280, 20, hWnd, NULL, g_hInst, NULL);

            g_prec_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                          WS_CHILD | WS_VISIBLE | ES_NUMBER,
                                          15, 40, 280, 24, hWnd,
                                          (HMENU)IDC_RESULT, g_hInst, NULL);
            wchar_t init[8];
            _snwprintf(init, 8, L"%d", g_app.precision >= 0 ? g_app.precision : 2);
            SetWindowTextW(g_prec_edit, init);

            CreateWindowExW(0, L"BUTTON", L"确定",
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            130, 80, 80, 28, hWnd,
                            (HMENU)IDOK, g_hInst, NULL);
            CreateWindowExW(0, L"BUTTON", L"自动",
                            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                            220, 80, 75, 28, hWnd,
                            (HMENU)IDCANCEL, g_hInst, NULL);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                wchar_t buf[8];
                GetWindowTextW(g_prec_edit, buf, 8);
                wchar_t *end = NULL;
                long value = wcstol(buf, &end, 10);
                if (*buf == L'\0' || end == NULL || *end != L'\0' ||
                    value < 0 || value > 15) {
                    MessageBoxW(hWnd, L"请输入 0~15 的整数。",
                                L"精度设置", MB_OK | MB_ICONWARNING);
                    return 0;
                }
                g_prec_value = (int)value;
                g_prec_cancelled = FALSE;
                DestroyWindow(hWnd);
                return 0;
            }
            if (id == IDCANCEL) {
                g_prec_value = PRECISION_AUTO;
                g_prec_cancelled = FALSE;
                DestroyWindow(hWnd);
                return 0;
            }
            return 0;
        }

        case WM_CLOSE:
            g_prec_cancelled = TRUE;
            DestroyWindow(hWnd);
            return 0;

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static void ShowPrecisionDialog(void) {
    g_prec_value = -2;
    g_prec_cancelled = FALSE;

    g_prec_dialog = CreateWindowExW(0, L"CalcPrecisionClass", L"显示精度设置",
                                    WS_CAPTION | WS_SYSMENU,
                                    CW_USEDEFAULT, CW_USEDEFAULT,
                                    330, 160, g_app.hWnd, NULL, g_hInst, NULL);
    ShowWindow(g_prec_dialog, SW_SHOW);
    UpdateWindow(g_prec_dialog);
    EnableWindow(g_app.hWnd, FALSE);

    MSG msg;
    while (IsWindow(g_prec_dialog)) {
        if (GetMessageW(&msg, NULL, 0, 0) > 0) {
            if (!IsDialogMessageW(g_prec_dialog, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }

    EnableWindow(g_app.hWnd, TRUE);
    SetForegroundWindow(g_app.hWnd);

    if (!g_prec_cancelled && g_prec_value != -2) {
        g_app.precision = g_prec_value;
        if (g_app.has_ans) {
            wchar_t formatted[128];
            wchar_t label[160];
            FormatResult(g_app.ans, g_app.precision, formatted, 128);
            _snwprintf(label, 160, L"= %ls", formatted);
            label[159] = L'\0';
            SetWindowTextW(g_app.hResult, label);
        }
    }
}

/* ------------------------------------------------------------------ */
/* 控件与按钮                                                           */
/* ------------------------------------------------------------------ */

static HWND CreateCalcButton(HWND parent, int id, const wchar_t *label,
                             int x, int y, int w, int h) {
    return CreateWindowExW(0, L"BUTTON", label,
                           WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                           x, y, w, h, parent, (HMENU)(INT_PTR)id,
                           g_hInst, NULL);
}

typedef struct {
    int id;
    const wchar_t *token;   /* NULL 表示特殊按钮 */
} ButtonToken;

static const ButtonToken g_button_tokens[] = {
    {IDB_BASE + 10, L"sin("},  {IDB_BASE + 11, L"cos("},
    {IDB_BASE + 12, L"tan("},  {IDB_BASE + 13, L"sqrt("},
    {IDB_BASE + 14, L"ln("},   {IDB_BASE + 15, L"log("},
    {IDB_BASE + 20, L"7"},     {IDB_BASE + 21, L"8"},
    {IDB_BASE + 22, L"9"},     {IDB_BASE + 23, L"/"},
    {IDB_BASE + 24, L"4"},     {IDB_BASE + 25, L"5"},
    {IDB_BASE + 26, L"6"},     {IDB_BASE + 27, L"*"},
    {IDB_BASE + 30, L"1"},     {IDB_BASE + 31, L"2"},
    {IDB_BASE + 32, L"3"},     {IDB_BASE + 33, L"-"},
    {IDB_BASE + 34, L"("},     {IDB_BASE + 35, L")"},
    {IDB_BASE + 40, L"0"},     {IDB_BASE + 41, L"."},
    {IDB_BASE + 42, L"π"},     {IDB_BASE + 43, L"+"},
    {IDB_BASE + 44, L"e"},     {IDB_BASE + 45, L"Ans"},
    {IDB_BASE + 50, L"^"},     {IDB_BASE + 51, L"!"},
    {IDB_BASE + 52, L"log2("}, {IDB_BASE + 53, L"pow("},
    {IDB_BASE + 54, L"exp("},  {IDB_BASE + 55, L"abs("},
};

static const wchar_t *TokenForButton(int id) {
    for (size_t i = 0; i < sizeof(g_button_tokens) / sizeof(g_button_tokens[0]); i++) {
        if (g_button_tokens[i].id == id) {
            return g_button_tokens[i].token;
        }
    }
    return NULL;
}

static void CreateAllButtons(HWND parent) {
    const int left = 10;
    const int top = 82;
    const int col_w = 80;
    const int row_h = 50;
    const int gap = 4;

#define POS_X(c)   ((left) + (c) * ((col_w) + (gap)))
#define POS_Y(r)   ((top) + (r) * ((row_h) + (gap)))
#define SPAN_W(n)  ((n) * (col_w) + ((n) - 1) * (gap))

    /* 第 0 行：三角函数和常用函数 */
    CreateCalcButton(parent, IDB_BASE + 10, L"sin", POS_X(0), POS_Y(0), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 11, L"cos", POS_X(1), POS_Y(0), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 12, L"tan", POS_X(2), POS_Y(0), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 13, L"√", POS_X(3), POS_Y(0), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 14, L"ln", POS_X(4), POS_Y(0), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 15, L"log", POS_X(5), POS_Y(0), col_w, row_h);

    /* 第 1 行：7 8 9 / C */
    CreateCalcButton(parent, IDB_BASE + 20, L"7", POS_X(0), POS_Y(1), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 21, L"8", POS_X(1), POS_Y(1), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 22, L"9", POS_X(2), POS_Y(1), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 23, L"÷", POS_X(3), POS_Y(1), col_w, row_h);
    CreateCalcButton(parent, IDB_CLEAR, L"C", POS_X(4), POS_Y(1), SPAN_W(2), row_h);

    /* 第 2 行：4 5 6 * ⌫ */
    CreateCalcButton(parent, IDB_BASE + 24, L"4", POS_X(0), POS_Y(2), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 25, L"5", POS_X(1), POS_Y(2), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 26, L"6", POS_X(2), POS_Y(2), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 27, L"×", POS_X(3), POS_Y(2), col_w, row_h);
    CreateCalcButton(parent, IDB_BACK, L"⌫", POS_X(4), POS_Y(2), SPAN_W(2), row_h);

    /* 第 3 行：1 2 3 - ( ) */
    CreateCalcButton(parent, IDB_BASE + 30, L"1", POS_X(0), POS_Y(3), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 31, L"2", POS_X(1), POS_Y(3), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 32, L"3", POS_X(2), POS_Y(3), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 33, L"−", POS_X(3), POS_Y(3), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 34, L"(", POS_X(4), POS_Y(3), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 35, L")", POS_X(5), POS_Y(3), col_w, row_h);

    /* 第 4 行：0 . π + e Ans */
    CreateCalcButton(parent, IDB_BASE + 40, L"0", POS_X(0), POS_Y(4), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 41, L".", POS_X(1), POS_Y(4), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 42, L"π", POS_X(2), POS_Y(4), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 43, L"+", POS_X(3), POS_Y(4), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 44, L"e", POS_X(4), POS_Y(4), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 45, L"Ans", POS_X(5), POS_Y(4), col_w, row_h);

    /* 第 5 行：^ ! log2 pow = */
    CreateCalcButton(parent, IDB_BASE + 50, L"x^y", POS_X(0), POS_Y(5), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 51, L"n!", POS_X(1), POS_Y(5), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 52, L"log2", POS_X(2), POS_Y(5), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 53, L"pow", POS_X(3), POS_Y(5), col_w, row_h);
    CreateCalcButton(parent, IDB_EQUALS, L"=", POS_X(4), POS_Y(5), SPAN_W(2), row_h);

    /* 第 6 行：exp abs 精度 */
    CreateCalcButton(parent, IDB_BASE + 54, L"exp", POS_X(0), POS_Y(6), col_w, row_h);
    CreateCalcButton(parent, IDB_BASE + 55, L"abs", POS_X(1), POS_Y(6), col_w, row_h);
    CreateCalcButton(parent, IDB_PREC, L"精度", POS_X(2), POS_Y(6), SPAN_W(4), row_h);

#undef POS_X
#undef POS_Y
#undef SPAN_W
}

/* ------------------------------------------------------------------ */
/* 编辑框子类化：Enter 计算，Esc 退出                                    */
/* ------------------------------------------------------------------ */

LRESULT CALLBACK EditSubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            EvaluateExpression();
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            DestroyWindow(g_app.hWnd);
            return 0;
        }
    }
    return CallWindowProcW(g_oldEditProc, hWnd, msg, wParam, lParam);
}

/* ------------------------------------------------------------------ */
/* 主窗口过程                                                           */
/* ------------------------------------------------------------------ */

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_app.hWnd = hWnd;

            CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                            10, 10, 500, 28, hWnd, (HMENU)IDC_EDIT,
                            g_hInst, NULL);
            g_app.hEdit = GetDlgItem(hWnd, IDC_EDIT);

            CreateWindowExW(0, L"STATIC", L"",
                            WS_CHILD | WS_VISIBLE | SS_LEFT,
                            10, 46, 500, 26, hWnd, (HMENU)IDC_RESULT,
                            g_hInst, NULL);
            g_app.hResult = GetDlgItem(hWnd, IDC_RESULT);

            CreateAllButtons(hWnd);

            /* 子类化编辑框，拦截 Enter 和 Esc */
            g_oldEditProc = (WNDPROC)(LONG_PTR)SetWindowLongPtrW(
                g_app.hEdit, GWLP_WNDPROC, (LONG_PTR)EditSubclassProc);

            SetFocus(g_app.hEdit);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (HIWORD(wParam) == BN_CLICKED) {
                const wchar_t *token = TokenForButton(id);
                if (token != NULL) {
                    AppendToken(token);
                    return 0;
                }
                if (id == IDB_EQUALS) {
                    EvaluateExpression();
                    return 0;
                }
                if (id == IDB_CLEAR) {
                    ClearAll();
                    return 0;
                }
                if (id == IDB_BACK) {
                    Backspace();
                    return 0;
                }
                if (id == IDB_PREC) {
                    ShowPrecisionDialog();
                    return 0;
                }
            }
            return 0;
        }

        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }
            return DefWindowProcW(hWnd, msg, wParam, lParam);

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

/* ------------------------------------------------------------------ */
/* 入口                                                                 */
/* ------------------------------------------------------------------ */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    g_hInst = hInstance;

    memset(&g_app, 0, sizeof(g_app));
    g_app.precision = PRECISION_AUTO;

    /* 主窗口类 */
    WNDCLASSEXW wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"CalculatorMainClass";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"主窗口类注册失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    /* 精度弹窗类 */
    WNDCLASSEXW pc;
    memset(&pc, 0, sizeof(pc));
    pc.cbSize = sizeof(pc);
    pc.lpfnWndProc = PrecisionProc;
    pc.hInstance = hInstance;
    pc.lpszClassName = L"CalcPrecisionClass";
    pc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    pc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    pc.hIcon = LoadIconW(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&pc)) {
        MessageBoxW(NULL, L"精度窗口类注册失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    /* 固定大小主窗口（内容区约 520×430） */
    RECT rc = {0, 0, 520, 430};
    AdjustWindowRect(&rc, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                     FALSE);

    HWND hWnd = CreateWindowExW(0, L"CalculatorMainClass",
                                L"Calculator v3.0.0 (Windows GUI)",
                                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                rc.right - rc.left, rc.bottom - rc.top,
                                NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        MessageBoxW(NULL, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
