#!/usr/bin/env bash
# 在 WSL 里用 MSYS2 交叉编译 Calculator 到 Windows，采用“算法库分离”架构：
# 每个算法(核心/单位/矩阵/复数)编译成独立 .dll，主程序链接它们。
# 用法：./scripts/build_windows.sh
# 前置：WSL 侧挂载 MSYS2（默认 D:\MSYS2），MSYS2 内已装
#       mingw-w64-x86_64-gcc / mingw-w64-x86_64-gtk4 / mingw-w64-x86_64-pkgconf。
set -euo pipefail

MSYS2_BASH="/mnt/d/MSYS2/usr/bin/bash.exe"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MSYS_PROJECT="//wsl.localhost/Arch${ROOT}"

if [ ! -x "$MSYS2_BASH" ]; then
    echo "未找到 MSYS2 bash：$MSYS2_BASH" >&2
    exit 1
fi

"$MSYS2_BASH" -lc "
export MSYSTEM=MINGW64
export PATH=/mingw64/bin:\$PATH
export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig
export PKG_CONFIG_LIBDIR=/mingw64/lib/pkgconfig
cd '$MSYS_PROJECT'
mkdir -p bin gui
echo ':: 编译算法库 (.dll)'
# 复数库无依赖，先编；核心库依赖复数，需链入
x86_64-w64-mingw32-gcc -shared -Iinclude src/complex.c   -o bin/libcalc_complex.dll -Wl,--out-implib,bin/libcalc_complex.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/units.c     -o bin/libcalc_units.dll   -Wl,--out-implib,bin/libcalc_units.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/matrix.c    -o bin/libcalc_matrix.dll  -Wl,--out-implib,bin/libcalc_matrix.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/db.c        -o bin/libcalc_db.dll     -Wl,--out-implib,bin/libcalc_db.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/calculator.c -o bin/libcalc_core.dll  -Wl,--out-implib,bin/libcalc_core.dll.a -Lbin -lcalc_complex
echo ':: 编译 CLI -> bin/Calculator.exe'
x86_64-w64-mingw32-gcc -Iinclude src/main.c -o bin/Calculator.exe -Lbin -lcalc_core -lcalc_units -lcalc_matrix -lcalc_complex -lcalc_db
echo ':: 编译 GTK4 GUI -> gui/Calculator-gui.exe'
# -mwindows：生成 GUI 子系统可执行文件，双击不弹出终端窗口
x86_64-w64-mingw32-gcc -Iinclude gui/gui_gtk4.c -o gui/Calculator-gui.exe -mwindows -Lbin -lcalc_core -lcalc_matrix -lcalc_complex -lcalc_db \$(pkg-config --cflags --libs gtk4)
echo ':: 剥离符号(提高反编译门槛)'
for f in bin/*.dll bin/Calculator.exe gui/Calculator-gui.exe; do strip "\$f" 2>/dev/null || true; done
echo ':: 完成。产物：'
ls -la bin/*.dll bin/Calculator.exe gui/Calculator-gui.exe
"
