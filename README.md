# 🧮 Calculator —— C 语言科学计算器

一个使用 **C11** 编写的科学计算器项目。

- `main` 分支：跨平台表达式求值核心 + 连续运行命令行界面（本 README）
- `linux` 分支：GTK3 图形界面（按钮 + 键盘输入）
- `windows` 分支：Win32 图形界面（按钮 + 键盘输入）

## ✨ v3.1.0 功能

| 类别 | 支持内容 |
|------|----------|
| 基本运算 | `+` 加、`-` 减、`*` 乘、`/` 除 |
| 乘方 | `^`（右结合），也可用 `pow(a, b)` |
| 开根 | `sqrt(x)` |
| 对数 | `ln(x)` 自然对数、`log(x)` 以 10 为底、`log2(x)` 以 2 为底 |
| 阶乘 | `n!`（后缀写法，如 `5!`） |
| 三角函数（弧度制） | `sin(x)`、`cos(x)`、`tan(x)` |
| 三角函数（角度制） | `sind(x)`、`cosd(x)`、`tand(x)` |
| 其他函数 | `exp(x)`、`abs(x)` |
| 常量 | `pi`（或 `π`）、`e`、`Ans` |
| 连续运行 | 按 `Enter` 计算，按 `Esc` 退出，不退出程序 |
| 记忆功能 | `Ans` 保存上一次计算结果，可继续参与运算 |
| 显示精度 | `precision N` 保留 N 位小数；`precision auto` 恢复自动格式 |
| 错误处理 | 除数为 0、负数开根、对数真数非法等都会给出明确提示 |

## 🛠️ 构建

### 方式一：直接使用 gcc / cc（最简单）

```bash
cd Calculator
make          # 编译，生成 bin/Calculator
make run      # 编译并启动
make test     # 编译并运行全部自动测试
make clean    # 清理编译产物
```

### 方式二：CMake

```bash
cd Calculator
mkdir -p build && cd build
cmake ..
cmake --build .
ctest            # 运行测试
../bin/Calculator
```

## 📖 使用方法

### 交互模式

```bash
./bin/Calculator
```

进入后可以**连续计算**：

```text
> e^2
= 7.38905609893065
> sin(Ans)
= 0.893854657425058
> precision 2
显示精度已设置为：保留 2 位小数。
> 1/3
= 0.33
> Ans+1
= 1.33
> 1/0
[ERROR] 除法错误：除数不能为 0
表达式：1/0
         ^
> （按 Esc）
再见！
```

> 说明：`precision` 只改变屏幕显示，`Ans` 内部始终保存完整精度。

### 命令行直接计算

```bash
./bin/Calculator "2^10" "Ans+1"
./bin/Calculator --precision 4 "e^2" "sin(Ans)"
```

## 📝 表达式书写规则

- **数字**：支持小数（`3.14`、`.5`）和科学计数法（`1e-3`）。
- **运算符优先级**（从高到低）：
  1. `!`（阶乘）
  2. `^`（乘方，右结合：`2^3^2 = 2^(3^2) = 512`）
  3. 一元 `+` / `-`
  4. `*` / `/`
  5. `+` / `-`
  - 括号可以改变优先级。
  - 按数学惯例：`-3^2 = -(3^2) = -9`；需要 `(-3)^2` 时请加括号。
- **函数**：函数名后必须跟括号，例如 `sin(pi/2)`、`sqrt(9)`、`pow(2,10)`。
- **常量**：直接写 `pi`、`π`、`e` 或 `Ans`。
- **暂不支持省略乘号**：请写 `2*pi`，不要写 `2pi`。
- **定义域检查（本项目暂不涉及虚数）**：
  - 除数不能为 `0`
  - `sqrt` 的输入不能为负数
  - `ln` / `log` / `log2` 的真数必须大于 `0`
  - 阶乘只接受 `0~170` 的非负整数
  - 负数底数的乘方只允许整数指数（如 `(-2)^2`）

## 📂 项目结构

```text
Calculator/
├── CMakeLists.txt          # CMake 构建脚本
├── Makefile                # make 构建脚本
├── README.md
├── include/
│   └── calculator.h        # 表达式求值核心 API（含 Ans 版本）
├── src/
│   ├── calculator.c        # 词法分析 + 递归下降语法分析 + 求值
│   └── main.c              # 连续运行 REPL / 命令行入口
└── tests/
    └── test_calculator.c   # 自动测试
```

### 维护者说明

- `calc_evaluate()`：旧版兼容接口，不支持 `Ans`。
- `calc_evaluate_with_ans()`：完整接口，传入上一次结果后可使用 `Ans`。
- 解析器采用**递归下降**方法，文法写在 `src/calculator.c` 文件头部。
- 新增函数时：
  1. 在 `unary_function()` 中增加定义域检查和 `math.h` 调用；
  2. 在 `tests/test_calculator.c` 中补充测试用例；
  3. 运行 `make test` 确认无误。
- 结果使用 `double` 存储，满足日常科学计算需求；需要任意精度时需引入额外大数库。

## 🧪 测试

```bash
make test
```

测试覆盖：加、减、乘、除、乘方、开根、对数、阶乘、`sin/cos/tan`、角度制三角函数、`pi/π/e` 常量、`Ans` 记忆、优先级、右结合乘方，以及各类非法输入。
