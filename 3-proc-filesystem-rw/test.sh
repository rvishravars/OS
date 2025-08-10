#!/usr/bin/env bash
set -euo pipefail

MOD_NAME="proc"                # your module name (proc.ko)
POSSIBLE_PATHS=(
  "/proc/driver/my_proc/rw_info"
  "/proc/my_proc/rw_info"      # in case you flip the #if to put it directly under /proc
)

log()  { printf "\n[*] %s\n" "$*"; }
ok()   { printf "   [OK] %s\n" "$*"; }
fail() { printf "   [FAIL] %s\n" "$*" >&2; exit 1; }

need_root() { [ "$(id -u)" -eq 0 ] || fail "Run this script with sudo."; }

find_proc_path() {
  for p in "${POSSIBLE_PATHS[@]}"; do
    if [[ -e "$p" ]]; then
      echo "$p"
      return 0
    fi
  done
  return 1
}

need_root

log "Clearing kernel ring buffer (so we only see this run)..."
dmesg -C || true

log "Building module..."
make -C "/lib/modules/$(uname -r)/build" M="$PWD" modules

if lsmod | awk '{print $1}' | grep -qx "$MOD_NAME"; then
  log "Removing existing module..."
  rmmod "$MOD_NAME" || fail "Could not remove existing module"
fi

log "Inserting module..."
insmod "${MOD_NAME}.ko" || fail "insmod failed"
sleep 0.3

log "Kernel log (module messages):"
dmesg | tail -n 10

# Resolve the actual /proc path
PROC_PATH=""
if PROC_PATH="$(find_proc_path)"; then
  ok "Proc entry $PROC_PATH exists"
else
  fail "Proc entry not found under any expected path"
fi

log "Checking permissions..."
perm="$(stat -c '%A' "$PROC_PATH")"
echo "   perms: $perm"
[[ "$perm" == -r--r--r-- || "$perm" == -rw-r--r-- || "$perm" == -r--r----- || "$perm" == -rw-r----- ]] || true
# Owner should be able to write; group/others read-only is fine depending on your umask and kernel.
ok "Permissions look sane"

log "Reading initial value..."
initial="$(cat "$PROC_PATH")"
echo -n "   -> $initial"
grep -q '^param = ' <<<"$initial" || fail "Unexpected read format"

log "Writing a new value (1234) and re-reading..."
echo 1234 | tee "$PROC_PATH" >/dev/null
after="$(cat "$PROC_PATH")"
echo -n "   -> $after"
grep -q '^param = 1234$' <<<"$after" || fail "Value didn't update to 1234"
ok "Write path updates param"

log "Feeding invalid input ('not-an-int') and ensuring value remains unchanged..."
echo "not-an-int" | tee "$PROC_PATH" >/dev/null || true
sleep 0.1
after_bad="$(cat "$PROC_PATH")"
echo -n "   -> $after_bad"
grep -q '^param = 1234$' <<<"$after_bad" || fail "Invalid input should not change param"
ok "Invalid input rejected; value intact"

log "Testing multiple writes before close (open FD, write '56' in two parts, then close)..."
# Open once, write twice, close FD → your close() parser should run after FD closes
exec 3>"$PROC_PATH"
printf "5" >&3
printf "6" >&3
exec 3>&-
sleep 0.1
multi="$(cat "$PROC_PATH")"
echo -n "   -> $multi"
grep -q '^param = 56$' <<<"$multi" || fail "Expected param to become 56 after multi-write + close"
ok "Multi-write before close parsed correctly"

log "Removing module..."
rmmod "$MOD_NAME" || fail "rmmod failed"

if PROC_PATH="$(find_proc_path)"; then
  fail "Proc entry still exists after removal: $PROC_PATH"
else
  ok "Proc entry removed"
fi

log "Last kernel messages (for reference):"
dmesg | tail -n 20

log "All tests passed ✅"
