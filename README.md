# 🧮 Calculator —— C 语言科学计算器（Windows GUI 版）

本分支为 **Windows GUI** 版本，使用 Win32 API 实现独立计算器窗口。

- 本分支：`windows` —— Win32 图形界面
- 主分支：`main` —— 跨平台核心 + 命令行连续运行版
- `linux` 分支 —— GTK3 图形界面

## ✨ 功能

- 加、减、乘、除、乘方 `^`、开根 `sqrt`
- 对数 `ln` / `log` / `log2`，阶乘 `!`
- 三角函数：弧度制 `sin/cos/tan`，角度制 `sind/cosd/tand`
- 常量：`pi` / `π` / `e` / `Ans`
- **独立按钮**：数字、运算符、函数都有对应按钮
- **键盘输入**：输入框可以直接打字
- **连续运行**：`Enter` 计算，`Esc` 或关闭窗口退出
- **Ans 记忆**：`Ans` 引用上一次结果，例如先算 `e^2`，再算 `sin(Ans)`
- **显示精度**：点击“精度”按钮设置保留 0~15 位小数，或恢复自动格式
- **错误提示**：除数为 0、负数开根等错误会弹窗提示，程序不退出

## 🛠️ 构建（Windows）

### 方式一：一键脚本（MinGW-w64）

安装 MSYS2 的 MinGW-w64 工具链，把 `mingw64\bin` 加入 `PATH`，然后：

```bat
build_windows.bat
```

生成的程序为 `bin\Calculator.exe`。

### 方式二：手动编译

```bat
mkdir bin
gcc -std=c11 -O2 -Wall -Wextra -Iinclude ^
    -municode -mwindows ^
    src\gui_windows.c src\calculator.c ^
    -o bin\Calculator.exe -lm
```

### 方式三：CMake（MinGW Makefiles 或 Visual Studio）

```powershell
mkdir build; cd build
cmake .. -G "MinGW Makefiles"
cmake --build .
ctest
..\bin\Calculator.exe
```

### 在 WSL/Linux 上交叉编译验证

```bash
make windows   # 使用 x86_64-w64-mingw32-gcc 交叉编译三个 .exe
make test      # Linux 下运行核心测试
```

## 📖 使用方法

1. 运行 `bin\Calculator.exe`
2. 直接键盘输入表达式，或点击数字/运算符/函数按钮
3. 按 **Enter** 或点击 **=** 计算
4. 结果显示在输入框下方；按 **Esc** 或关闭窗口退出

### 记忆功能示例

```text
输入 e^2            → = 7.38905609893065
清空后输入 sin(Ans) → = 0.893854657425058
```

### 精度设置示例

点击“精度”按钮：
- 输入 `2`，再计算 `1/3`，显示 `= 0.33`
- 点击“自动”，恢复完整格式
- 精度只影响显示，`Ans` 内部保存完整精度

## 📂 项目结构

```text
Calculator/
├── CMakeLists.txt
├── Makefile
├── build_windows.bat       # Windows 一键编译脚本
├── README.md
├── include/calculator.h    # 求值核心 API
├── src/
│   ├── calculator.c        # 词法分析 + 递归下降求值
│   ├── main.c              # 命令行版（Calculator-cli）
│   └── gui_windows.c       # Win32 图形界面
└── tests/test_calculator.c
```

## 🧪 测试

- Linux/WSL：`make test`
- Windows（MinGW Shell）：`gcc -std=c11 -Iinclude tests\test_calculator.c src\calculator.c -o bin\calculator_tests.exe -lm && bin\calculator_tests.exe`
