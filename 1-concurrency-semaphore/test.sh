#!/bin/sh
# POSIX sh test for lab1_semaphore{1,2,3}
set -eu

MOD1=lab1_semaphore1
MOD2=lab1_semaphore2
MOD3=lab1_semaphore3

log()  { printf "\n[*] %s\n" "$*"; }
ok()   { printf "   [OK] %s\n" "$*"; }
warn() { printf "   [!] %s\n" "$*"; }

try_insmod() {
  ko="$1.ko"
  if sudo insmod "$ko" 2>/tmp/insmod.err; then
    ok "insmod $ko"
    return 0
  else
    rc=$?
    err=$(tr -d '\n' </tmp/insmod.err || true)
    warn "insmod $ko failed (rc=$rc): $err"
    return "$rc"
  fi
}

safe_rmmod() {
  mod="$1"
  if grep -q "^$mod " /proc/modules 2>/dev/null; then
    if sudo rmmod "$mod"; then
      ok "rmmod $mod"
    else
      warn "rmmod $mod failed"
    fi
  fi
}

show_logs() {
  # Broaden match to both semaphore- and semaphore-based messages
  {
    dmesg | tail -n 200 | \
    grep -E "lab1_(semaphore)|\b(semaphore)\b.*(busy|acquired|released|UNLOCKED|LOCKED)"
  } || true
}

probe_proc_state() {
  if [ -r /proc/my_semaphore_state ]; then
    log "Current /proc/my_semaphore_state:"
    cat /proc/my_semaphore_state || true
  fi
}

# Build if Makefile exists
if [ -f Makefile ]; then
  log "Building modules…"
  make
fi

# Ensure .ko files exist
for f in "$MOD1.ko" "$MOD2.ko" "$MOD3.ko"; do
  [ -f "$f" ] || { echo "Missing $f"; exit 1; }
done

log "Clean slate (unloading any old modules)…"
safe_rmmod "$MOD3"
safe_rmmod "$MOD2"
safe_rmmod "$MOD1"

log "Step 1: Load exporter ($MOD1)…"
try_insmod "$MOD1" || { echo "Could not load $MOD1"; exit 1; }

log "Step 2: Load consumer ($MOD2) — should SUCCEED and hold the semaphore…"
try_insmod "$MOD2" || true
probe_proc_state
show_logs

log "Step 3: Try to load $MOD3 — should FAIL with -EBUSY…"
try_insmod "$MOD3" || ok "$MOD3 correctly refused to load while semaphore is held"
probe_proc_state
show_logs

log "Step 4: Switch holders — unload $MOD2, load $MOD3, then try $MOD2 again…"
safe_rmmod "$MOD2"
try_insmod "$MOD3" || { echo "Expected $MOD3 to load after $MOD2 unload"; exit 1; }

log "Now $MOD2 should FAIL with -EBUSY…"
try_insmod "$MOD2" || ok "$MOD2 refused to load while $MOD3 holds the semaphore"
probe_proc_state
show_logs

log "Cleanup…"
safe_rmmod "$MOD2"
safe_rmmod "$MOD3"
safe_rmmod "$MOD1"
ok "All done."
