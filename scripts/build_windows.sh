#!/usr/bin/env bash
# 在 WSL 里用 MSYS2 交叉编译 Calculator 到 Windows（CLI + GTK4 GUI）。
# 用法：./scripts/build_windows.sh
# 前置：WSL 侧挂载 MSYS2（默认 D:\MSYS2），MSYS2 内已装
#       mingw-w64-x86_64-gcc / mingw-w64-x86_64-gtk4 / mingw-w64-x86_64-pkgconf。
set -euo pipefail

MSYS2_BASH="/mnt/d/MSYS2/usr/bin/bash.exe"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# 把 WSL 绝对路径转成 MSYS2 可见的 //wsl.localhost/Arch/... 路径
MSYS_PROJECT="//wsl.localhost/Arch${ROOT}"

if [ ! -x "$MSYS2_BASH" ]; then
    echo "未找到 MSYS2 bash：$MSYS2_BASH" >&2
    echo "请先安装 MSYS2 到 D:\\MSYS2，并安装 mingw-w64 工具链与 gtk4。" >&2
    exit 1
fi

"$MSYS2_BASH" -lc "
export MSYSTEM=MINGW64
export PATH=/mingw64/bin:\$PATH
export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig
export PKG_CONFIG_LIBDIR=/mingw64/lib/pkgconfig
cd '$MSYS_PROJECT'
echo ':: 交叉编译 CLI -> bin/Calculator.exe'
x86_64-w64-mingw32-gcc -Iinclude src/main.c src/calculator.c src/units.c -o bin/Calculator.exe
echo ':: 交叉编译 GTK4 GUI -> gui/Calculator-gui.exe'
x86_64-w64-mingw32-gcc -Iinclude gui/gui_gtk4.c src/calculator.c \$(pkg-config --cflags --libs gtk4) -o gui/Calculator-gui.exe
echo ':: 完成。产物：'
ls -la bin/Calculator.exe gui/Calculator-gui.exe
"
