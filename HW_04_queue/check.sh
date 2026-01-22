#!/usr/bin/env bash
set -euo pipefail

KO=./kernel_fifo.ko
MOD=kernel_fifo
DIR="/sys/module/${MOD}/parameters"

cleanup() { sudo rmmod "$MOD" >/dev/null 2>&1 || true; }
trap cleanup EXIT

[[ -f "$KO" ]] || { echo "ERROR: $KO not found. Run make." >&2; exit 1; }

sudo rmmod "$MOD" >/dev/null 2>&1 || true
sudo dmesg -C >/dev/null 2>&1 || true

sudo insmod "$KO" max_size=8

for f in max_size enqueue dequeue peek size available is_empty is_full clear; do
  [[ -e "$DIR/$f" ]] || { echo "ERROR: missing $DIR/$f"; exit 2; }
done

echo 10 | sudo tee "$DIR/enqueue" >/dev/null
echo 20 | sudo tee "$DIR/enqueue" >/dev/null
echo 30 | sudo tee "$DIR/enqueue" >/dev/null

sz="$(tr -d '\n' < "$DIR/size")"
[[ "$sz" == "3" ]] || { echo "ERROR: size expected 3 got $sz"; exit 3; }

peek="$(tr -d '\n' < "$DIR/peek")"
[[ "$peek" == "10" ]] || { echo "ERROR: peek expected 10 got $peek"; exit 4; }

dq="$(tr -d '\n' < "$DIR/dequeue")"
[[ "$dq" == "10" ]] || { echo "ERROR: dequeue expected 10 got $dq"; exit 5; }

sz2="$(tr -d '\n' < "$DIR/size")"
[[ "$sz2" == "2" ]] || { echo "ERROR: size expected 2 got $sz2"; exit 6; }

echo 1 | sudo tee "$DIR/clear" >/dev/null
empty="$(tr -d '\n' < "$DIR/is_empty")"
[[ "$empty" == "1" ]] || { echo "ERROR: is_empty expected 1 got $empty"; exit 7; }

sudo rmmod "$MOD"
echo "OK"
