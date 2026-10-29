#!/bin/sh
HERE="$(dirname "$0")"
EXTDIR="$(cd "$HERE/.." && pwd)"
cd "$EXTDIR" || exit 1

export GAMBATTE_HOME="$EXTDIR"
export LD_LIBRARY_PATH="$EXTDIR/lib:$EXTDIR/bin:$LD_LIBRARY_PATH"

# Ensure X11 display is set for GTK on Kindle
[ -z "$DISPLAY" ] && export DISPLAY=:0

# Kill any lingering instances to prevent input or device lockup
killall -9 gambatte-k2 2>/dev/null || true

# Prevent screensaver while playing
lipc-set-prop -i com.lab126.powerd preventScreenSaver 1 2>/dev/null || true

# Ensure binary has executable permissions
[ -f "$EXTDIR/bin/gambatte-k2" ] && chmod +x "$EXTDIR/bin/gambatte-k2"
[ -f "$EXTDIR/gambatte-k2" ] && chmod +x "$EXTDIR/gambatte-k2"
[ -f "$EXTDIR/gambatte-k2-armel" ] && chmod +x "$EXTDIR/gambatte-k2-armel"
[ -f "$EXTDIR/gambatte-k2-armhf" ] && chmod +x "$EXTDIR/gambatte-k2-armhf"

# Rotate log file if it exceeds 256KB
LOGFILE="$EXTDIR/gambatte.log"
if [ -f "$LOGFILE" ]; then
    LOGSIZE=$(wc -c < "$LOGFILE" 2>/dev/null || echo 0)
    if [ "$LOGSIZE" -gt 262144 ]; then
        tail -n 500 "$LOGFILE" > "$LOGFILE.tmp" 2>/dev/null && mv "$LOGFILE.tmp" "$LOGFILE"
    fi
fi

echo "=== [$(date)] Starting Gambatte-K2 ===" >> "$LOGFILE"
echo "EXTDIR=$EXTDIR" >> "$LOGFILE"
echo "DISPLAY=$DISPLAY" >> "$LOGFILE"
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >> "$LOGFILE"

# Clear e-ink screen before launching to remove KUAL ghosting
which eips >/dev/null 2>&1 && eips -c 2>/dev/null || true

# Run gambatte-k2
if [ -f "$EXTDIR/bin/gambatte-k2" ]; then
    "$EXTDIR/bin/gambatte-k2" >> "$LOGFILE" 2>&1
    RET=$?
elif [ -f "$EXTDIR/gambatte-k2" ]; then
    "$EXTDIR/gambatte-k2" >> "$LOGFILE" 2>&1
    RET=$?
else
    echo "ERROR: binary gambatte-k2 not found!" >> "$LOGFILE"
    RET=127
fi

echo "=== [$(date)] Gambatte-K2 exited with code $RET ===" >> "$LOGFILE"

# Nếu có lỗi thoát khác 0, hiển thị thông báo lên màn hình Kindle trong 3 giây
if [ "$RET" -ne 0 ]; then
    which eips >/dev/null 2>&1 && eips 0 10 "Gambatte loi $RET: Xem gambatte.log" 2>/dev/null || true
    sleep 3
fi

# Clean up: clear screen and restore screensaver when exiting
which eips >/dev/null 2>&1 && eips -c 2>/dev/null || true
lipc-set-prop -i com.lab126.powerd preventScreenSaver 0 2>/dev/null || true
