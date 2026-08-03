# 🧮 C语言计算器 (Calculator)

一个用于学习C语言、CMake构建系统和动态库（DLL）的迷你计算器项目。

## ✨ 当前功能
- 加法、减法、乘法、除法
- 使用动态库 (`libmath.dll`) 分离数学运算逻辑
- 醒目的错误提示（除零检测、非法输入）

## 🛠️ 构建方法

确保你已安装 **CMake** 和 **MinGW**。

```bash
# 1. 进入项目目录
cd Calculator

# 2. 创建构建文件夹
mkdir build && cd build

# 3. 配置并编译
cmake ..
cmake --build .

# 4. 运行程序（可执行文件和DLL位于 bin/ 目录下）
cd ../bin
./Calculator.exe