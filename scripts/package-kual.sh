#!/usr/bin/env bash
set -euo pipefail

echo "=== Đóng gói KUAL Extension Gambatte-K2 cho Kindle PW2/PW3 ==="

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
DIST_DIR="$ROOT_DIR/dist/gambatte-k2"
OUTPUT_ZIP="$ROOT_DIR/gambatte-k2-kual.zip"

rm -rf "$ROOT_DIR/dist" "$OUTPUT_ZIP"
mkdir -p "$DIST_DIR/bin" "$DIST_DIR/cores" "$DIST_DIR/lib"

# 1. Sao chép cấu hình KUAL
cp "$ROOT_DIR/extension/gambatte-k2/config.xml" "$DIST_DIR/"
cp "$ROOT_DIR/extension/gambatte-k2/menu.json" "$DIST_DIR/"
cp "$ROOT_DIR/extension/gambatte-k2/bin/start.sh" "$DIST_DIR/bin/"
chmod +x "$DIST_DIR/bin/start.sh"

# 2. Sao chép cấu hình emulator
if [ -f "$ROOT_DIR/config.ini" ]; then
    cp "$ROOT_DIR/config.ini" "$DIST_DIR/"
fi

# 3. Sao chép binary thực thi
BIN_SRC="${1:-}"
if [ -n "$BIN_SRC" ] && [ -f "$BIN_SRC" ]; then
    cp "$BIN_SRC" "$DIST_DIR/bin/gambatte-k2"
    chmod +x "$DIST_DIR/bin/gambatte-k2"
    echo "  ✓ Đã nạp binary từ đối số: $BIN_SRC"
elif [ -f "$ROOT_DIR/bin/gambatte-k2" ]; then
    cp "$ROOT_DIR/bin/gambatte-k2" "$DIST_DIR/bin/gambatte-k2"
    chmod +x "$DIST_DIR/bin/gambatte-k2"
    echo "  ✓ Đã nạp binary: $ROOT_DIR/bin/gambatte-k2"
elif [ -f "$ROOT_DIR/builddir_kindlehf/gambatte-k2" ]; then
    cp "$ROOT_DIR/builddir_kindlehf/gambatte-k2" "$DIST_DIR/bin/gambatte-k2"
    chmod +x "$DIST_DIR/bin/gambatte-k2"
    echo "  ✓ Đã nạp binary: $ROOT_DIR/builddir_kindlehf/gambatte-k2"
else
    echo "  [ERROR] Không tìm thấy binary gambatte-k2!"
    exit 1
fi

# 4. Sao chép libretro cores
if [ -d "$ROOT_DIR/cores" ]; then
    cp -r "$ROOT_DIR/cores/." "$DIST_DIR/cores/" 2>/dev/null || true
    echo "  ✓ Đã nạp libretro cores"
fi

# 5. Đóng gói ZIP chuẩn /mnt/us/extensions/gambatte-k2
cd "$ROOT_DIR/dist"
zip -r "$OUTPUT_ZIP" gambatte-k2/
cd "$ROOT_DIR"
rm -rf "$ROOT_DIR/dist"

echo "=========================================================="
echo "  ĐÓNG GÓI THÀNH CÔNG: $OUTPUT_ZIP"
if [ -f "$OUTPUT_ZIP" ]; then
    ls -lh "$OUTPUT_ZIP"
fi
echo "=========================================================="
