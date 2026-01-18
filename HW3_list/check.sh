#!/usr/bin/env bash
set -euo pipefail

KO=./kernel_stack.ko
DIR=/sys/kernel/kernel_stack

cleanup() {
  sudo rmmod kernel_stack >/dev/null 2>&1 || true
}
trap cleanup EXIT

if [[ ! -f "$KO" ]]; then
  echo "ERROR: $KO not found. Run make first." >&2
  exit 1
fi

sudo rmmod kernel_stack >/dev/null 2>&1 || true
sudo dmesg -C >/dev/null 2>&1 || true

sudo insmod "$KO"

[[ -d "$DIR" ]] || { echo "ERROR: $DIR not found"; exit 2; }

# push 10,20,30
echo 10 | sudo tee "$DIR/push" >/dev/null
echo 20 | sudo tee "$DIR/push" >/dev/null
echo 30 | sudo tee "$DIR/push" >/dev/null

sz="$(cat "$DIR/size" | tr -d '\n')"
[[ "$sz" == "3" ]] || { echo "ERROR: size expected 3 got $sz"; exit 3; }

peek="$(cat "$DIR/peek" | tr -d '\n')"
[[ "$peek" == "30" ]] || { echo "ERROR: peek expected 30 got $peek"; exit 4; }

pop="$(cat "$DIR/pop" | tr -d '\n')"
[[ "$pop" == "30" ]] || { echo "ERROR: pop expected 30 got $pop"; exit 5; }

sz2="$(cat "$DIR/size" | tr -d '\n')"
[[ "$sz2" == "2" ]] || { echo "ERROR: size expected 2 got $sz2"; exit 6; }

echo 1 | sudo tee "$DIR/clear" >/dev/null

empty="$(cat "$DIR/is_empty" | tr -d '\n')"
[[ "$empty" == "1" ]] || { echo "ERROR: is_empty expected 1 got $empty"; exit 7; }

sudo rmmod kernel_stack

echo "OK"
