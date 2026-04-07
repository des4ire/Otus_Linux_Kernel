#!/usr/bin/env bash
set -euo pipefail

KO=./kernel_sync_demo.ko
MOD=kernel_sync_demo
DIR="/sys/module/${MOD}/parameters"

cleanup(){ sudo rmmod "$MOD" >/dev/null 2>&1 || true; }
trap cleanup EXIT

[[ -f "$KO" ]] || { echo "ERROR: $KO not found"; exit 1; }

sudo rmmod "$MOD" >/dev/null 2>&1 || true
sudo dmesg -C >/dev/null 2>&1 || true

sudo insmod "$KO" num_threads=8 iterations=5000 lock_type=0

for f in run result lock_type stats reset; do
  [[ -e "$DIR/$f" ]] || { echo "ERROR: missing $DIR/$f"; exit 2; }
done

# run spinlock
echo 1 | sudo tee "$DIR/run" >/dev/null
cat "$DIR/result"
cat "$DIR/stats"

# switch mutex
echo 1 | sudo tee "$DIR/lock_type" >/dev/null
echo 1 | sudo tee "$DIR/run" >/dev/null
cat "$DIR/result"
cat "$DIR/stats"

# switch semaphore
echo 2 | sudo tee "$DIR/lock_type" >/dev/null
echo 1 | sudo tee "$DIR/run" >/dev/null
cat "$DIR/result"
cat "$DIR/stats"

# reset
echo 1 | sudo tee "$DIR/reset" >/dev/null
cat "$DIR/result" >/dev/null

sudo rmmod "$MOD"
echo OK
