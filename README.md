# 🧮 Calculator —— C 语言科学计算器（Linux GTK 图形版）

本分支为 **Linux GUI** 版本，使用 GTK3 实现独立计算器窗口。

- 本分支：`linux` —— GTK3 图形界面
- 主分支：`main` —— 跨平台核心 + 命令行连续运行版
- `windows` 分支 —— Win32 图形界面

## ✨ 功能

- 加、减、乘、除、乘方 `^`、开根 `sqrt`
- 对数 `ln` / `log` / `log2`，阶乘 `!`
- 三角函数：弧度制 `sin/cos/tan`，角度制 `sind/cosd/tand`
- 常量：`pi` / `π` / `e` / `Ans`
- **独立按钮**：数字、运算符、函数都有对应按钮
- **键盘输入**：输入框可以直接打字，数字也可以完全不用鼠标
- **连续运行**：`Enter` 计算，`Esc` 或关闭窗口退出
- **Ans 记忆**：`Ans` 引用上一次结果，例如先算 `e^2`，再算 `sin(Ans)`
- **显示精度**：点击“精度”按钮设置保留 0~15 位小数，或恢复自动格式
- **错误提示**：除数为 0、负数开根等错误会弹窗提示，程序不退出

## 🛠️ 构建

需要 GTK3 开发库：

```bash
sudo apt install libgtk-3-dev   # Debian / Ubuntu
```

然后：

```bash
cd Calculator
make            # 生成 bin/Calculator（GUI）和 bin/Calculator-cli（命令行）
make run        # 启动 GUI
make run-cli    # 启动命令行版
make gui-test   # GUI 自检
make test       # 核心引擎全部自动测试
make clean
```

也可以使用 CMake：

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
ctest
../bin/Calculator
```

## 📖 使用方法

1. 启动 GUI：`make run`
2. 可以直接在输入框键入表达式，也可以点击按钮输入
3. 输入后按 **Enter** 或点击 **=**
4. 结果显示在输入框下方；按 **Esc** 退出

### 记忆功能示例

```text
输入 e^2         → = 7.38905609893065
清空后输入 sin(Ans) → = 0.893854657425058
```

### 精度设置示例

点击“精度”：
- 选择 `2`，再计算 `1/3`，显示 `= 0.33`
- 选择“自动”，恢复完整格式
- 说明：精度只影响显示，`Ans` 内部保存完整精度

## 📂 项目结构

```text
Calculator/
├── CMakeLists.txt
├── Makefile
├── README.md
├── include/calculator.h       # 求值核心 API
├── src/
│   ├── calculator.c           # 词法分析 + 递归下降求值
│   ├── main.c                 # 命令行版（Calculator-cli）
│   └── gui_linux.c            # GTK3 图形界面（Calculator）
└── tests/test_calculator.c
```

## 🧪 测试

```bash
make test       # 49 个核心测试
make gui-test   # GUI 自检（验证 GUI 与核心正确链接）
```
