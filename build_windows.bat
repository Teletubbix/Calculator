@echo off
REM Calculator v3.0.0 Windows GUI 一键编译脚本
REM 需要 MinGW-w64 GCC，并已加入 PATH（例如 MSYS2 的 mingw64\bin）

if not exist bin mkdir bin
gcc -std=c11 -O2 -Wall -Wextra -Iinclude ^
    -municode -mwindows ^
    src\gui_windows.c src\calculator.c ^
    -o bin\Calculator.exe -lm

if %ERRORLEVEL% EQU 0 (
    echo.
    echo 编译成功：bin\Calculator.exe
    echo 双击运行，或在命令行执行：bin\Calculator.exe
) else (
    echo.
    echo 编译失败，请检查 gcc 是否已安装并加入 PATH。
)
pause
