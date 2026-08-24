# 🧮 Calculator —— C 语言科学计算器（通信/理工科专用）

一个使用 **C11** 编写、面向**通信工程/理工科**的科学计算器，同时提供**命令行（CLI）**与 **GTK4 图形界面（GUI，Linux/Windows 双端）**。
支持**完整复数（含相量极坐标）**、**dB 功率级换算**、**矩阵**、**单位换算**，以及 v6.0 新增的**数值工具（积分/求导/求根/求和/连乘）**。

> 采用「**算法库分离**」架构：每个算法是独立动态库（`.so`/`.dll`），可单独编译、单独修复。
> 源码以 **GNU AGPL-3.0** 发布。

---

## ✨ 功能总览（v6.0.4）

### 表达式求值（内置 + 插件注册）

| 类别 | 支持内容 |
|------|----------|
| 基本运算 | `+` `-` `*` `/`、括号、一元正负 |
| 乘方 | `^`（右结合：`2^3^2 = 512`），也可用 `pow(a,b)` |
| 开根 / 对数 | `sqrt`、`ln`（自然）、`log`（以10为底）、`log2`、`logn(x,b)`（任意底） |
| 阶乘 | `n!`（后缀，如 `5!`）、`comb(n,k)`、`perm(n,k)` |
| 三角 | `sin/cos/tan`（受角度模式影响）、`sind/cosd/tand`（恒为角度制） |
| 反三角 | `asin/acos/atan`（以及角度版 `asind/acosd/atand`） |
| 双曲 | `sinh`、`cosh`、`tanh` |
| 取整/其他 | `floor`、`ceil`、`round`、`trunc`、`abs`、`sign`、`exp`、`atan2(y,x)`、`mod(a,b)`、`gcd`、`lcm` |
| 常量 | `pi`（或 `π`）、`e`、`tau`、`phi`、`Ans`（上次结果）、`x`（自变量） |

### 复数 / 相量（通信核心）

- 完整虚数单位字面量：`3+4j`、`4j`、`2i`、`j`、`i`
- 复数四则运算：`(1+2j)*(3-4j)`、`(3+4j)/(1-2j)`、`(1+1j)^2`
- 复数函数：`sin/cos/sqrt/ln/exp/pow` 等对复数同样适用
- 模/辐角：`cabs(实,虚)` 求模、`carg(实,虚)` 求辐角（随角度模式）
- **极坐标（相量）**：`r∠θ` 或 `r@θ`（`∠` 与 `@` 等价），角度随模式——如 `3∠30`、`5∠(pi/2)*2`、`abs(3∠(pi/3))`

### dB / 功率级（通信）

`dbm(mW)`、`mw(dBm)`、`dbw(W)`、`w(dBW)`、`pow2db(功率比)`、`db2pow(dB)`。

### 矩阵（方阵，考试常用）

`det2(a,b,c,d)` / `det3(9参)` 求行列式，`trace2(a,b,c,d)` / `trace3(9参)` 求迹（元素按行优先）。

### 单位换算（convert 命令）

`convert <值> <源> <目标>`：长度 / 质量 / 温度 / 数据存储 / 时间 / 速度 / 功率 / 能量 / 频率 / 压强 / dB。
例：`convert 1 km m`、`convert 0 degC degF`、`convert 0 dBm mW`、`convert 3 GHz MHz`。

### 数值工具（v6.0，表达式用 `x` 作自变量）

把表达式看成函数 `f(x)`，在 CLI 中直接算：

| 命令 | 含义 | 示例 |
|------|------|------|
| `integrate a b 表达式` | 数值定积分 `∫[a,b] f(x) dx` | `integrate 0 1 x^2` → 0.333… |
| `nderiv x0 表达式` | 数值导数 `f'(x0)` | `nderiv 3 x^2` → 6 |
| `root a b 表达式` | 在 `[a,b]` 内找 `f(x)=0` 的实根 | `root 0 5 x^2-4` → 2 |
| `sum a b 表达式` | 求和 `Σ_{i=a}^{b} f(i)` | `sum 1 5 x^2` → 55 |
| `product a b 表达式` | 连乘 `Π_{i=a}^{b} f(i)` | `product 1 5 x` → 120 |

> 相关引擎 API：`calc_evaluate_x`、`calc_ninteg`、`calc_nderiv`、`calc_root`、`calc_sum`、`calc_prod`。

### 其他

| 功能 | 说明 |
|------|------|
| 角度模式 | `mode deg` / `rad` / `grad`，影响 `sin/cos/tan` 与 `carg`、以及相量 `∠` |
| 显示精度 | `precision N`（N 位小数）、`precision auto`（自动格式） |
| 历史记录 | `history` 查看、`clear` 清空；上/下箭头翻看历史（`Ans` 记忆上次结果） |
| 错误处理 | 除数 0、sqrt 负数、ln 非正、阶乘越界等均给出明确中文提示 |

### 图形界面（GTK4，Linux 原生 + Windows 交叉编译）

- **7 套二次元（原神）高对比主题**：`蒙德 风`、`璃月 岩`、`稻妻 雷`、`须弥 草`、`枫丹 水`、`纳塔 火`、`至冬 冰`，一键循环切换
- **所有符号统一近黑、浅色按键底**，杜绝“白底白字”看不清问题
- 顶部 **Genshin 大幅徽标**融入窗口背景；主题可在纯渐变上叠加**动漫背景图**（放入 `themes/img/<主题>.png` 即生效）
- 显示切换：**a+bj（直角）↔ R∠θ（极坐标）**；**标准 / 科学 / 工程** 记数法
- 角度切换：**DEG / RAD** 按钮
- 有序的 6 列分组布局，A-Z/数字/函数/运算清晰分区；`Ans`、常量、单位等一键输入

---

## 🧩 架构（模块化 / 分布式）

每个算法都是**独立的动态库**（Linux `.so` / Windows `.dll`），互不依赖：

| 库 | 内容 |
|----|------|
| `libcalc_core` | 表达式解析与求值引擎 + 插件式函数注册表 |
| `libcalc_units` | 单位换算引擎 |
| `libcalc_matrix` | 矩阵（行列式 / 迹）|
| `libcalc_complex` | 复数（模 / 辐角）|
| `libcalc_db` | dB / 功率级换算 |

主程序（CLI/GUI）通过 `calc_register_functions()`（见 `include/calculator.h`）把各库注册进引擎。
**某个算法出 bug，只改对应源文件、重编该库即可**，不影响其它部分。

核心语法采用**递归下降**解析，复数值贯穿始终；`x` 变量为 v6.0 数值工具提供自变量。

---

## 🛠️ 构建

### 方式一：Makefile（最简）

```bash
cd Calculator
make          # 编译，生成 bin/Calculator（CLI）
make run      # 编译并启动
make test     # 编译并运行全部自动测试
make clean    # 清理
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

### 方式三：Linux 原生 GUI（GTK4）

```bash
# 需安装 GTK4 开发包：Debian/Ubuntu: libgtk-4-dev；Arch: gtk4
cd Calculator
cc -Iinclude gui/gui_gtk4.c -o bin/Calculator-gui-linux \
   -Lbin -lcalc_core -lcalc_complex -lcalc_db -lcalc_matrix \
   $(pkg-config --cflags --libs gtk4) -Wl,-rpath,"$(pwd)/bin" -lm
./bin/Calculator-gui-linux
```

（也可用 CMake，检测到 GTK4 会自动构建 `Calculator-gui` 目标。）

### 方式四：交叉编译到 Windows（WSL 里用 MSYS2）

```bash
./scripts/build_windows.sh
# 输出 bin/Calculator.exe 与 gui/Calculator-gui.exe
```

- CLI 输出 `bin/Calculator.exe`，GUI 输出 `gui/Calculator-gui.exe`（已用 `-mwindows` 编译，**双击不弹终端窗口**）。
- 依赖：WSL 侧挂载 MSYS2（本方案 `D:\MSYS2`），MSYS2 内已装 `mingw-w64-x86_64-gcc`、`mingw-w64-x86_64-gtk4`、`mingw-w64-x86_64-pkgconf`。
- **绿色分发**：把 `gui/Calculator-gui.exe` 连同所有 `lib*.dll`、`share/`、`lib/`、`assets/` 一起放到同一目录（`D:\calcview` 已就绪），双击 `Calculator-gui.exe` 即可运行，无需再装 GTK。

---

## 📖 使用方法

### CLI 交互模式

```bash
./bin/Calculator
```

```text
> e^2
= 7.38905609893065
> sin(Ans)
= 0.893854657425058
> precision 2
显示精度已设置为：保留 2 位小数。
> 1/3
= 0.33
> (1+2j)*(3-4j)          ← 复数
= 11+2j
> 3∠30                   ← 相量（角度制）
= 2.59808+1.5i
> convert 0 dBm mW        ← 单位换算
= 1 mW
> integrate 0 1 x^2       ← 数值积分
= 0.333333333333333
> root 0 5 x^2-4          ← 求根
= 2
> 1/0
[ERROR] 除法错误：除数不能为 0
表达式：1/0
         ^
> （按 Esc）
再见！
```

**命令**：`help`（帮助）、`precision N/auto`、`mode deg/rad/grad`、`convert V from to`、`history`/`hist`、`clear`/`cls`、`integrate`、`nderiv`、`root`、`sum`、`product`、`quit`/`exit`/`q`。

### CLI 命令行直接计算

```bash
./bin/Calculator "2^10" "Ans+1"
./bin/Calculator --mode deg "3∠30"          # 指定角度制
./bin/Calculator --precision 4 "e^2"
./bin/Calculator --version
```

### GUI 使用

1. 双击 `Calculator-gui.exe`（Windows）或运行 `./bin/Calculator-gui-linux`（Linux）。
2. 顶部输入框输入表达式，回车或点 `=` 计算；结果显示在下方，`Ans` 自动保存上次结果。
3. 顶部 `DEG/RAD` 切换角度；`a+bj / R∠θ` 切换直角/极坐标显示；`标准/科学/工程` 切换记数法。
4. 右下主题按钮循环切换 7 套主题。

---

## 📝 表达式书写规则

- **数字**：支持小数（`3.14`、`.5`）与科学计数（`1e-3`）。
- **优先级**（从高到低）：`!` 阶乘 → `^` 乘方（右结合）→ 一元 `+/-` → `* /` → `+ -`；括号可改变优先级。按数学惯例 `-3^2 = -9`，需要 `(-3)^2` 请加括号。
- **函数**：后必须跟括号，如 `sin(pi/2)`、`pow(2,10)`。
- **常量**：`pi`/`π`、`e`、`tau`、`phi`、`Ans`、`x`、`j`/`i`（虚数单位）。
- **暂不支持省略乘号**：写 `2*pi`，不要写 `2pi`。
- **复数**：引擎已支持虚数（`sqrt(-1)` 会返回 `1i`；`ln` 对负数等也按复数处理），因此 `sqrt` 的负数输入不再报错。
- **定义域检查**：除数不能为 0；`ln/log/log2` 的真数需 > 0；阶乘只接受 0~170 的非负整数；`pow2db` 功率比需为正、`dbm` 需非负等。

---

## 📂 项目结构

```text
Calculator/
├── CMakeLists.txt
├── Makefile
├── README.md
├── LICENSE                 # GNU AGPL-3.0
├── include/
│   └── calculator.h        # 引擎 API + 函数注册表 + 数值工具 API
├── src/
│   ├── calculator.c        # 词法/语法/求值核心 + x 变量 + 积分/求导/求根/求和/连乘
│   ├── units.c             # 单位换算
│   ├── matrix.c            # 矩阵（行列式/迹）
│   ├── complex.c           # 复数（模/辐角）
│   ├── db.c                # dB/功率级
│   └── main.c              # REPL / 命令行入口
├── gui/
│   └── gui_gtk4.c          # GTK4 图形界面（主题/徽标/显示切换）
├── scripts/
│   └── build_windows.sh    # MSYS2 交叉编译到 Windows
├── themes/
│   └── img/                # 主题背景图（放入 <主题>.png 即叠加）
├── assets/
│   └── logo.png            # Genshin 徽标（背景暗纹）
└── tests/
    └── test_calculator.c   # 自动测试
```

---

## 🧪 测试

```bash
make test
```

覆盖：基本运算、优先级、右结合乘方、常量、`Ans`、复数（字面量/四则/复数函数）、极坐标 `∠`/`@`、`cabs/carg`、`det/trace` 矩阵、dB 函数、单位换算、`x` 变量、数值积分/求导/求根/求和/连乘，以及各类非法输入。

---

## 🔧 维护者说明

- API 分层：`calc_evaluate`（旧版兼容，无 `Ans`）、`calc_evaluate_with_ans`、`calc_evaluate_mode`、`calc_evaluate_complex`（复数结果）、以及 v6.0 数值工具 `calc_evaluate_x/calc_ninteg/calc_nderiv/calc_root/calc_sum/calc_prod`。
- 新增函数：
  1. 在 `src/calculator.c` 的对应函数分派处增加实现与定义域检查；
  2. 在 `tests/test_calculator.c` 补充测试；
  3. 运行 `make test` 确认。
- 结果以 `double` 存储；需要任意精度时需另引入大数库。
- 每个算法库独立维护，单库改动不影响其它模块。
