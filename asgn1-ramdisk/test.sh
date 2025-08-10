#!/usr/bin/env bash
set -euo pipefail

DEV_NAME="asgn1"
DEV_PATH="/dev/$DEV_NAME"
MOD_NAME="ramdiskext"
MAJOR=""

log()  { echo -e "\n[*] $*"; }
ok()   { echo "   [OK] $*"; }
fail() { echo "   [FAIL] $*" >&2; exit 1; }

need_root() { [ "$(id -u)" -eq 0 ] || fail "Run this script with sudo."; }

need_root

log "Building module..."
make

log "Removing old module (if loaded)..."
if lsmod | grep -q "^$MOD_NAME"; then
    rmmod $MOD_NAME || fail "Could not remove $MOD_NAME"
fi

log "Inserting module..."
insmod ${MOD_NAME}.ko || fail "insmod failed"
sleep 0.5

log "Getting major number..."
MAJOR=$(awk "\$2==\"$DEV_NAME\" {print \$1}" /proc/devices)
[ -n "$MAJOR" ] || fail "Could not find major number for $DEV_NAME"

log "Creating device node..."
rm -f "$DEV_PATH"
mknod "$DEV_PATH" c "$MAJOR" 0
chmod 666 "$DEV_PATH"

log "Testing write/read..."
echo "Hello, Ramdisk!" > "$DEV_PATH"
readback=$(head -c 15 "$DEV_PATH")
[ "$readback" = "Hello, Ramdisk!" ] && ok "Basic write/read" || fail "Write/read failed"

log "Testing seek and partial read..."
echo -n "ABCDEFGHIJ" > "$DEV_PATH"
dd if="$DEV_PATH" bs=1 skip=3 count=4 2>/dev/null | grep -q "DEFG" && ok "Seek/read" || fail "Seek/read failed"

log "Testing truncate on O_WRONLY open..."
echo -n "12345" > "$DEV_PATH"
exec 3>"$DEV_PATH"   # open write-only, should truncate
exec 3>&-
[ ! -s "$DEV_PATH" ] && ok "Truncate on O_WRONLY open" || fail "Truncate failed"

log "Cleaning up..."
rm -f "$DEV_PATH"
rmmod $MOD_NAME