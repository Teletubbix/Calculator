#!/usr/bin/env bash
# 在 Linux 环境：编译 CLI + GTK4 GUI，并把可执行文件、说明文档打成 tar.gz。
# 用法：bash scripts/package_linux.sh <仓库路径> <版本号>
set -euo pipefail

REPO="${1:?需要仓库路径}"
VERSION="${2:?需要版本号，如 v6.0.5}"

cd "$REPO"
mkdir -p bin dist

echo "== 1/3 编译（make）=="
make -j"$(nproc)" bin/Calculator

echo "== 2/3 编译 GTK4 GUI =="
cc -Iinclude gui/gui_gtk4.c -o bin/Calculator-gui-linux \
  -Lbin -lcalc_core -lcalc_complex -lcalc_db -lcalc_matrix \
  $(pkg-config --cflags --libs gtk4) -Wl,-rpath,"$(pwd)/bin" -lm

echo "== 3/3 打包 =="
BUNDLE="$(mktemp -d)"
trap 'rm -rf "$BUNDLE"' EXIT
cp -f bin/Calculator bin/Calculator-gui-linux bin/libcalc_*.so "$BUNDLE/"
cp -f assets/logo.png "$BUNDLE/logo.png"
cp -f README.md "$BUNDLE/README.md"
cp -f LICENSE   "$BUNDLE/LICENSE"
[ -f "docs/使用说明.md" ] && cp -f "docs/使用说明.md" "$BUNDLE/使用说明.md"
[ -f "themes/img/README.md" ] && cp -f "themes/img/README.md" "$BUNDLE/README-themes.md"
cat > "$BUNDLE/快速开始.txt" <<'EOF'
Calculator <版本> (Linux)
  0. 先看《使用说明.md》快速上手。
  1. CLI:  ./Calculator
  2. GUI:  ./Calculator-gui-linux   (需 libgtk-4)
  3. logo.png 放可执行文件同级作窗口背景暗纹。
libcalc_*.so 需与可执行文件同目录 (已设 rpath)。
EOF

out="dist/Calculator-${VERSION}-linux.tar.gz"
tar czf "$REPO/$out" -C "$BUNDLE" .
echo "打包完成：$out"
ls -la "$out"
