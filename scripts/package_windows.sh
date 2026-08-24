#!/usr/bin/env bash
# 在 MSYS2 MINGW64 环境里：交叉编译 Windows 产物，并把 GTK4 运行库/资源/
# 说明书打进一个自包含的 zip（双击 Calculator-gui.exe 即可运行）。
#
# 用法：bash scripts/package_windows.sh <仓库MSYS路径> <版本号>
#   <仓库MSYS路径> 由调用方用 cygpath -u "$GITHUB_WORKSPACE" 得到，或 WSL 下传 /mnt/... 对应路径
set -euo pipefail

REPO="${1:?需要仓库路径}"
VERSION="${2:?需要版本号，如 v6.0.5}"
MSYSTEM=MINGW64
export PATH=/mingw64/bin:$PATH
export PKG_CONFIG_PATH=/mingw64/lib/pkgconfig
export PKG_CONFIG_LIBDIR=/mingw64/lib/pkgconfig
DL=/mingw64/bin

cd "$REPO"
mkdir -p bin gui dist
BUNDLE="$(mktemp -d)"
trap 'rm -rf "$BUNDLE"' EXIT

echo "== 1/5 编译算法库 (.dll) =="
x86_64-w64-mingw32-gcc -shared -Iinclude src/complex.c    -o bin/libcalc_complex.dll -Wl,--out-implib,bin/libcalc_complex.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/units.c      -o bin/libcalc_units.dll   -Wl,--out-implib,bin/libcalc_units.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/matrix.c     -o bin/libcalc_matrix.dll  -Wl,--out-implib,bin/libcalc_matrix.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/db.c         -o bin/libcalc_db.dll      -Wl,--out-implib,bin/libcalc_db.dll.a
x86_64-w64-mingw32-gcc -shared -Iinclude src/calculator.c -o bin/libcalc_core.dll   -Wl,--out-implib,bin/libcalc_core.dll.a -Lbin -lcalc_complex

echo "== 2/5 编译 CLI 与 GUI =="
x86_64-w64-mingw32-gcc -Iinclude src/main.c -o bin/Calculator.exe \
  -Lbin -lcalc_core -lcalc_units -lcalc_matrix -lcalc_complex -lcalc_db
x86_64-w64-mingw32-gcc -Iinclude gui/gui_gtk4.c -o gui/Calculator-gui.exe -mwindows \
  -Lbin -lcalc_core -lcalc_matrix -lcalc_complex -lcalc_db \
  $(pkg-config --cflags --libs gtk4)

echo "== 3/5 收集本工程产物 =="
cp -f bin/*.dll bin/Calculator.exe gui/Calculator-gui.exe "$BUNDLE/"

echo "== 4/5 递归解析 GTK 依赖并拷贝运行库 =="
# 只把 Windows 系统 DLL（无需附带）与 libcalc_*（本工程）排除
declare -A SEEN=()
walk() {
  local name="$1"
  [ -n "${SEEN[$name]:-}" ] && return 0
  SEEN[$name]=1
  local f="$DL/$name"
  [ -f "$f" ] || return 0   # 不在 /mingw64/bin 的是系统 DLL，直接跳过
  cp -f "$f" "$BUNDLE/$name" 2>/dev/null || true
  local deps
  deps=$(objdump -p "$f" 2>/dev/null | sed -n 's/^\s*DLL Name:\s*//p')
  for dep in $deps; do
    case "${dep,,}" in
      kernel32*|msvcrt*|advapi32*|comctl32*|comdlg32*|crypt32*|d3d11*|dcomp*|dwmapi*|gdi32*|hid*|user32*|shell32*|shlwapi*|ws2_32*|ole32*|oleaut32*|setupapi*|uxtheme*|version*|dxgi*|opengl32*|glu32*|wldap32*|userenv*|psapi*|powrprof*|ntdll*|imm32*|wtsapi32*|propsys*|bcrypt*|secur32*|winmm*|d2d1*|dwrite*|mf*|uuid*|windowscodecs*|winhttp*|wininet*|dnsapi*|iphlpapi*|netapi32*|mpr*|netuser*|ntmarta*|sensapi*|srclient*|wpcap*|libcalc_*|shcore*|usp10*|cfgmgr32*|msimg32*|gdiplus*|d3d12*|winspool*|rpcrt4*|oleacc*|combase*|uiautomationcore*) ;;
      *) walk "$dep" ;;
    esac
  done
}
walk libgtk-4-1.dll

echo "== 5/5 拷贝资源与说明书 =="
mkdir -p "$BUNDLE/lib" "$BUNDLE/share" "$BUNDLE/assets"
cp -rf "$DL/../lib/gdk-pixbuf-2.0" "$BUNDLE/lib/"
cp -rf /mingw64/share/glib-2.0 "$BUNDLE/share/"
cp -rf /mingw64/share/icons     "$BUNDLE/share/"
cp -rf /mingw64/share/gtk-4.0   "$BUNDLE/share/"
cp -f  assets/logo.png          "$BUNDLE/assets/logo.png"
cp -f  README.md                "$BUNDLE/README.md"
cp -f  LICENSE                  "$BUNDLE/LICENSE"
# Windows 下 zip 对中文文件名会乱码，故说明书用 ASCII 名 USER_GUIDE.md
if [ -f "docs/使用说明.md" ]; then cp -f "docs/使用说明.md" "$BUNDLE/USER_GUIDE.md"; fi

out="dist/Calculator-${VERSION}-windows.zip"
mkdir -p dist
# 先在 Windows 驱动上的临时目录压缩（MSYS2 对 //wsl.localhost 文件系统写 zip 会受限），再拷入 dist
TMPZIP="${TMPDIR:-/tmp}/Calculator-${VERSION}-windows.$$.zip"
( cd "$BUNDLE" && zip -q -r "$TMPZIP" . )
cp -f "$TMPZIP" "$out"
rm -f "$TMPZIP"
echo "打包完成：$out"
ls -la "$out"
