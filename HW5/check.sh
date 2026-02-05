#!/usr/bin/env bash
set -euo pipefail

KO=./kernel_alloc.ko
MOD=kernel_alloc
DIR="/sys/module/${MOD}/parameters"

cleanup() { sudo rmmod "$MOD" >/dev/null 2>&1 || true; }
trap cleanup EXIT

[[ -f "$KO" ]] || { echo "ERROR: $KO not found. Run make." >&2; exit 1; }

sudo rmmod "$MOD" >/dev/null 2>&1 || true
sudo dmesg -C >/dev/null 2>&1 || true

sudo insmod "$KO"

for f in alloc free stats bitmap_info; do
  [[ -e "$DIR/$f" ]] || { echo "ERROR: missing $DIR/$f"; exit 2; }
done

# 1) stats initial
cat "$DIR/stats" >/dev/null

# 2) allocate 8KiB and 16KiB
echo 8192  | sudo tee "$DIR/alloc" >/dev/null
echo 16384 | sudo tee "$DIR/alloc" >/dev/null

# взять адрес первого выделения из dmesg
ADDR1="$(sudo dmesg | grep -F "${MOD}: allocated 8192 bytes" | tail -n 1 | sed -n 's/.* at \(0x[0-9a-fA-F]\+\)$/\1/p')"
[[ -n "$ADDR1" ]] || { echo "ERROR: cannot parse first alloc addr"; exit 3; }

# 3) stats after alloc
cat "$DIR/stats" >/dev/null

# 4) free first
echo "$ADDR1" | sudo tee "$DIR/free" >/dev/null

# 5) stats after free
cat "$DIR/stats" >/dev/null

# 6) bitmap info exists
cat "$DIR/bitmap_info" >/dev/null

sudo rmmod "$MOD"
echo "OK"
