#!/bin/sh
# test.sh - Test script for the 'proc' kernel module
# Usage: sudo ./test.sh

set -eu

MOD_NAME="proc"
PROC_PATH="/proc/driver/my_proc/ro_info"

log() { printf "\n[*] %s\n" "$*"; }
ok()  { printf "   [OK] %s\n" "$*"; }
fail(){ printf "   [FAIL] %s\n" "$*" >&2; exit 1; }

# 1) Build the module
log "Building module..."
make || fail "Build failed"

# 2) Remove if already loaded
if lsmod | grep -q "^${MOD_NAME}\b"; then
    log "Removing existing module..."
    sudo rmmod "$MOD_NAME" || fail "Could not remove existing module"
fi

# 3) Insert the module
log "Inserting module..."
sudo insmod "${MOD_NAME}.ko" || fail "insmod failed"

# 4) Wait a moment and check dmesg
sleep 1
log "Kernel log (last 5 lines):"
dmesg | tail -n 5

# 5) Verify the /proc entry exists
if [ -f "$PROC_PATH" ]; then
    ok "Proc entry $PROC_PATH exists"
else
    fail "Proc entry $PROC_PATH not found"
fi

# 6) Read from the proc entry
log "Reading $PROC_PATH..."
cat "$PROC_PATH" || fail "Could not read $PROC_PATH"

# 7) Attempt to write (should fail)
log "Attempting to write to $PROC_PATH (should fail)..."
if echo 999 | sudo tee "$PROC_PATH" > /dev/null 2>&1; then
    fail "Write unexpectedly succeeded!"
else
    ok "Write failed as expected (read-only)"
fi

# 8) Remove the module
log "Removing module..."
sudo rmmod "$MOD_NAME" || fail "rmmod failed"

# 9) Verify removal
if [ ! -f "$PROC_PATH" ]; then
    ok "Proc entry removed"
else
    fail "Proc entry still exists after removal"
fi

log "Test completed successfully."
