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

# 3. Sao chép binary thực thi (cả bin/ và thư mục gốc để tương thích 100%)
BIN_SRC="${1:-}"
ACTUAL_BIN=""
if [ -n "$BIN_SRC" ] && [ -f "$BIN_SRC" ]; then
    ACTUAL_BIN="$BIN_SRC"
elif [ -f "$ROOT_DIR/builddir_kindlehf/gambatte-k2" ]; then
    ACTUAL_BIN="$ROOT_DIR/builddir_kindlehf/gambatte-k2"
elif [ -f "$ROOT_DIR/bin/gambatte-k2" ]; then
    ACTUAL_BIN="$ROOT_DIR/bin/gambatte-k2"
fi

if [ -n "$ACTUAL_BIN" ]; then
    cp "$ACTUAL_BIN" "$DIST_DIR/bin/gambatte-k2"
    cp -f "$ACTUAL_BIN" "$DIST_DIR/gambatte-k2"
    cp -f "$ACTUAL_BIN" "$DIST_DIR/gambatte-k2-armhf"
    cp -f "$ACTUAL_BIN" "$DIST_DIR/gambatte-k2-armel"
    chmod +x "$DIST_DIR/bin/gambatte-k2" "$DIST_DIR/gambatte-k2" "$DIST_DIR/gambatte-k2-armhf" "$DIST_DIR/gambatte-k2-armel"
    echo "  ✓ Đã nạp binary: $ACTUAL_BIN"
fi

# 4. Sao chép libretro cores (cả cores/, thư mục gốc và bin/)
if [ -d "$ROOT_DIR/cores" ]; then
    cp -r "$ROOT_DIR/cores/." "$DIST_DIR/cores/" 2>/dev/null || true
    if [ -f "$DIST_DIR/cores/gambatte_libretro.so" ] && [ ! -f "$DIST_DIR/cores/gambatte_libretro-armel.so" ]; then
        cp -f "$DIST_DIR/cores/gambatte_libretro.so" "$DIST_DIR/cores/gambatte_libretro-armel.so"
    fi
    if [ -f "$DIST_DIR/cores/gambatte_libretro.so" ] && [ ! -f "$DIST_DIR/cores/gambatte_libretro-armhf.so" ]; then
        cp -f "$DIST_DIR/cores/gambatte_libretro.so" "$DIST_DIR/cores/gambatte_libretro-armhf.so"
    fi
    cp -f "$DIST_DIR/cores/"* "$DIST_DIR/" 2>/dev/null || true
    cp -f "$DIST_DIR/cores/"* "$DIST_DIR/bin/" 2>/dev/null || true
    echo "  ✓ Đã nạp libretro cores đa đường dẫn (cores, root, bin)"
fi

if [ -f "$DIST_DIR/config.ini" ]; then
    cp -f "$DIST_DIR/config.ini" "$DIST_DIR/bin/config.ini" 2>/dev/null || true
fi

# 5. Đóng gói ZIP bằng python3 zipfile
python3 -c "
import os, zipfile
dist_base = '$ROOT_DIR/dist'
zip_path = '$OUTPUT_ZIP'
with zipfile.ZipFile(zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
    for root, dirs, files in os.walk(dist_base):
        for f in files:
            fp = os.path.join(root, f)
            arcname = os.path.relpath(fp, dist_base)
            zf.write(fp, arcname)
print('Package created:', zip_path)
"

rm -rf "$ROOT_DIR/dist"

echo "=========================================================="
echo "  ĐÓNG GÓI THÀNH CÔNG: $OUTPUT_ZIP"
ls -lh "$OUTPUT_ZIP"
echo "=========================================================="
