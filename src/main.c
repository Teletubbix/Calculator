// ============================================================
// 文件名: main.c (Windows GUI 版本)
// 说明:  使用 Win32 API 构建计算器窗口，调用 expr_eval 引擎
// 分支:  windows
// ============================================================

#include <windows.h>           // Windows API 核心
#include <stdio.h>             // sprintf
#include <stdlib.h>
#include "expr_eval.h"         // 我们的计算引擎

// 控件 ID 定义（用来区分窗口里的各个零件）
#define IDC_INPUT   101        // 输入框
#define IDC_BUTTON  102        // 计算按钮
#define IDC_RESULT  103        // 结果显示标签

// 全局实例句柄（Windows 需要这个来创建窗口）
HINSTANCE g_hInst;

// ------------------------------------------------------------
// 窗口过程函数（处理所有窗口消息，比如点击、键盘、重绘）
// ------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 窗口创建时，放上三个控件
            // 1. 输入框 (编辑框)
            CreateWindowEx(
                0, "EDIT", "", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT,
                10, 10, 300, 25,   // 位置 (10,10) 大小 300x25
                hWnd, (HMENU)IDC_INPUT, g_hInst, NULL
            );

            // 2. 计算按钮
            CreateWindowEx(
                0, "BUTTON", "计算", 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                320, 10, 60, 25,
                hWnd, (HMENU)IDC_BUTTON, g_hInst, NULL
            );

            // 3. 结果显示标签 (静态文本)
            CreateWindowEx(
                0, "STATIC", "结果: ", 
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                10, 45, 370, 25,
                hWnd, (HMENU)IDC_RESULT, g_hInst, NULL
            );
            break;
        }

        case WM_COMMAND: {
            // 当按钮被点击时，wParam 的低 16 位是控件 ID
            if (LOWORD(wParam) == IDC_BUTTON) {
                // 获取输入框的句柄
                HWND hInput = GetDlgItem(hWnd, IDC_INPUT);
                HWND hResult = GetDlgItem(hWnd, IDC_RESULT);

                // 读取用户输入的表达式（最多 255 个字符）
                char buffer[256] = {0};
                GetWindowText(hInput, buffer, 255);

                // 调用我们的计算引擎
                double result = evaluate_expression(buffer);

                // 把结果格式化，显示到结果标签上
                char output[256];
                snprintf(output, sizeof(output), "结果: %.4f", result);
                SetWindowText(hResult, output);
            }
            break;
        }

        case WM_DESTROY:
            // 点击窗口右上角 X 时，发送退出消息
            PostQuitMessage(0);
            break;

        default:
            // 不处理的消息交给 Windows 默认处理
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// ------------------------------------------------------------
// 程序入口（Windows GUI 程序的入口是 WinMain，而不是 main）
// ------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    // 1. 设计窗口类（就像画图纸）
    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WndProc;          // 指向窗口过程函数
    wc.hInstance     = hInstance;        // 当前程序实例
    wc.lpszClassName = "CalculatorClass"; // 类名（内部标识）
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW); // 鼠标指针样式
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // 背景色

    // 2. 注册窗口类（告诉 Windows 有这么一个模板）
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, "窗口类注册失败！", "错误", MB_ICONERROR);
        return 1;
    }

    // 3. 根据图纸创建真正的窗口
    HWND hWnd = CreateWindowEx(
        0, 
        "CalculatorClass",                 // 类名（必须跟注册的一致）
        "🧮 C语言计算器 (Windows GUI)",    // 窗口标题
        WS_OVERLAPPEDWINDOW,               // 窗口风格（带最大化、最小化按钮）
        CW_USEDEFAULT, CW_USEDEFAULT,      // 位置（系统默认）
        420, 120,                          // 宽度 420，高度 120
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) {
        MessageBox(NULL, "窗口创建失败！", "错误", MB_ICONERROR);
        return 1;
    }

    // 4. 显示窗口
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 5. 消息循环（Windows 程序的“心脏”，不停处理事件）
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); // 转换键盘消息
        DispatchMessage(&msg);  // 分发消息到窗口过程 WndProc
    }

    return 0;
}