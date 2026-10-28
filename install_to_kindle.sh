#!/usr/bin/env bash
# ==============================================================================
# Gambatte-K2: Trình cài đặt tự động an toàn cho Kindle Paperwhite 2/3
# ==============================================================================
set -euo pipefail

ZIP_FILE="/home/ka/Downloads/gambatte-k2-kual.zip"
KINDLE_DIR=""
KINDLE_DEV=""

echo "=== 1. Tìm kiếm và kết nối máy đọc sách Kindle PW3 ==="
# Tìm thiết bị có nhãn Kindle hoặc sda1
for dev in sda1 sdb1 sdc1; do
    if lsblk "/dev/$dev" >/dev/null 2>&1; then
        FSTYPE=$(lsblk -no FSTYPE "/dev/$dev" 2>/dev/null || true)
        if [ "$FSTYPE" = "vfat" ]; then
            KINDLE_DEV="$dev"
            break
        fi
    fi
done

if [ -z "$KINDLE_DEV" ]; then
    echo "⚠️ Chưa phát hiện ổ đĩa Kindle (có thể máy đang ở chế độ sạc hoặc đã rút cáp)."
    echo "👉 Vui lòng cắm lại cáp USB hoặc chọn 'USB Drive Mode' trên màn hình Kindle rồi chạy lại script này."
    exit 1
fi

DEV_PATH="/dev/$KINDLE_DEV"
# Sửa cờ dirty nếu cần và mount rw
udisksctl unmount -b "$DEV_PATH" 2>/dev/null || true
gdbus call --system --dest org.freedesktop.UDisks2 \
    --object-path "/org/freedesktop/UDisks2/block_devices/$KINDLE_DEV" \
    --method org.freedesktop.UDisks2.Filesystem.Repair "{}" 2>/dev/null || true
udisksctl mount -b "$DEV_PATH" >/dev/null 2>&1 || true

KINDLE_DIR=$(lsblk -no MOUNTPOINT "$DEV_PATH" 2>/dev/null | head -n 1 || true)
if [ -z "$KINDLE_DIR" ] || [ ! -d "$KINDLE_DIR" ]; then
    for path in "/run/media/$USER/Kindle" "/run/media/ka/Kindle" "/media/$USER/Kindle"; do
        if [ -d "$path" ]; then
            KINDLE_DIR="$path"
            break
        fi
    done
fi

if [ -z "$KINDLE_DIR" ] || [ ! -d "$KINDLE_DIR" ]; then
    echo "❌ Lỗi: Không thể mount phân vùng Kindle."
    exit 1
fi

echo "✓ Đã tìm thấy và mount Kindle tại: $KINDLE_DIR"

EXT_DIR="$KINDLE_DIR/extensions"
GAMBATTE_DIR="$EXT_DIR/gambatte-k2"
BACKUP_DIR="/tmp/gambatte_backup_$(date +%s)"

echo "=== 2. Sao lưu an toàn save state Pokémon ==="
mkdir -p "$BACKUP_DIR"
if [ -d "$GAMBATTE_DIR/state" ]; then
    cp -r "$GAMBATTE_DIR/state" "$BACKUP_DIR/"
    echo "✓ Đã bảo lưu các file save state: $(ls -1 "$BACKUP_DIR/state" | tr '
' ' ')"
fi
if [ -f "$GAMBATTE_DIR/cores/mgba_libretro-armhf.so" ]; then
    cp "$GAMBATTE_DIR/cores/mgba_libretro-armhf.so" "$BACKUP_DIR/" 2>/dev/null || true
elif [ -f "$GAMBATTE_DIR/mgba_libretro-armhf.so" ]; then
    cp "$GAMBATTE_DIR/mgba_libretro-armhf.so" "$BACKUP_DIR/" 2>/dev/null || true
fi

echo "=== 3. Cài đặt bản Gambatte-K2 mới nhất từ $ZIP_FILE ==="
rm -rf "$GAMBATTE_DIR"
python3 -c "
import zipfile
with zipfile.ZipFile('$ZIP_FILE', 'r') as zf:
    zf.extractall('$EXT_DIR')
"
echo "✓ Đã giải nén bản build mới vào $EXT_DIR/gambatte-k2"

echo "=== 4. Khôi phục save states và cấu hình tương thích ==="
if [ -d "$BACKUP_DIR/state" ]; then
    mkdir -p "$GAMBATTE_DIR/state"
    cp -r "$BACKUP_DIR/state/"* "$GAMBATTE_DIR/state/"
    echo "✓ Đã khôi phục toàn bộ save state vào $GAMBATTE_DIR/state"
fi
if [ -f "$BACKUP_DIR/mgba_libretro-armhf.so" ]; then
    cp "$BACKUP_DIR/mgba_libretro-armhf.so" "$GAMBATTE_DIR/cores/" 2>/dev/null || true
    cp "$BACKUP_DIR/mgba_libretro-armhf.so" "$GAMBATTE_DIR/" 2>/dev/null || true
fi

# Tương thích đa đường dẫn
cp -f "$GAMBATTE_DIR/cores/gambatte_libretro-armhf.so" "$GAMBATTE_DIR/gambatte_libretro-armhf.so" 2>/dev/null || true
cp -f "$GAMBATTE_DIR/cores/gambatte_libretro.so" "$GAMBATTE_DIR/gambatte_libretro.so" 2>/dev/null || true
cp -f "$GAMBATTE_DIR/bin/gambatte-k2" "$GAMBATTE_DIR/gambatte-k2" 2>/dev/null || true
cp -f "$GAMBATTE_DIR/bin/gambatte-k2" "$GAMBATTE_DIR/gambatte-k2-armhf" 2>/dev/null || true
chmod +x "$GAMBATTE_DIR/bin/gambatte-k2" "$GAMBATTE_DIR/gambatte-k2" "$GAMBATTE_DIR/gambatte-k2-armhf" 2>/dev/null || true
chmod +x "$GAMBATTE_DIR/bin/start.sh" 2>/dev/null || true
cp -f "$GAMBATTE_DIR/config.ini" "$GAMBATTE_DIR/bin/config.ini" 2>/dev/null || true

sync
echo "======================================================================"
echo "  ✅ CÀI ĐẶT THÀNH CÔNG GAMBATTE-K2 ĐÃ FIX LỖI INK WIDGETS!"
echo "======================================================================"
