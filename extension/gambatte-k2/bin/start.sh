#!/bin/sh
HERE="$(dirname "$0")"
EXTDIR="$(cd "$HERE/.." && pwd)"
cd "$EXTDIR" || exit 1

export GAMBATTE_HOME="$EXTDIR"
export LD_LIBRARY_PATH="$EXTDIR/lib:$LD_LIBRARY_PATH"

# Prevent screensaver while playing
lipc-set-prop -i com.lab126.powerd preventScreenSaver 1 2>/dev/null || true

# Ensure binary has executable permissions
[ -f "$EXTDIR/bin/gambatte-k2" ] && chmod +x "$EXTDIR/bin/gambatte-k2"

# Rotate log file if it exceeds 256KB
LOGFILE="$EXTDIR/gambatte.log"
if [ -f "$LOGFILE" ]; then
    LOGSIZE=$(wc -c < "$LOGFILE" 2>/dev/null || echo 0)
    if [ "$LOGSIZE" -gt 262144 ]; then
        tail -n 500 "$LOGFILE" > "$LOGFILE.tmp" 2>/dev/null && mv "$LOGFILE.tmp" "$LOGFILE"
    fi
fi

# Clear e-ink screen before launching to remove KUAL ghosting
which eips >/dev/null 2>&1 && eips -c 2>/dev/null || true

# Run gambatte-k2
"$EXTDIR/bin/gambatte-k2" >> "$LOGFILE" 2>&1

# Clean up: clear screen and restore screensaver when exiting
which eips >/dev/null 2>&1 && eips -c 2>/dev/null || true
lipc-set-prop -i com.lab126.powerd preventScreenSaver 0 2>/dev/null || true
