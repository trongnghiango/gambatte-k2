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
cp "$ROOT_DIR/config.ini" "$DIST_DIR/"

# 3. Sao chép binary thực thi
if [ -f "$ROOT_DIR/bin/gambatte-k2" ]; then
    cp "$ROOT_DIR/bin/gambatte-k2" "$DIST_DIR/bin/"
    chmod +x "$DIST_DIR/bin/gambatte-k2"
    echo "  ✓ Đã nạp binary: $ROOT_DIR/bin/gambatte-k2"
elif [ -f "$ROOT_DIR/gambatte-k2" ]; then
    cp "$ROOT_DIR/gambatte-k2" "$DIST_DIR/bin/"
    chmod +x "$DIST_DIR/bin/gambatte-k2"
    echo "  ✓ Đã nạp binary: $ROOT_DIR/gambatte-k2"
fi

# 4. Sao chép libretro cores nếu có
if [ -d "$ROOT_DIR/cores" ]; then
    cp -r "$ROOT_DIR/cores/"* "$DIST_DIR/cores/" 2>/dev/null || true
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
    echo "  Kích thước gói: $(ls -lh "$OUTPUT_ZIP" | awk '{print $5}')"
fi
echo "  Cách cài đặt: Giải nén vào thư mục /mnt/us/extensions/ trên Kindle"
echo "=========================================================="
