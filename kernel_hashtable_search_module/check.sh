#!/usr/bin/env bash
set -euo pipefail

KO=./kernel_hashtable_search.ko
MOD=kernel_hashtable_search
DIR="/sys/module/${MOD}/parameters"

cleanup(){ sudo rmmod "$MOD" >/dev/null 2>&1 || true; }
trap cleanup EXIT

[[ -f "$KO" ]] || { echo "ERROR: $KO not found"; exit 1; }

sudo rmmod "$MOD" >/dev/null 2>&1 || true
sudo dmesg -C >/dev/null 2>&1 || true

sudo insmod "$KO" array_size=1000 num_buckets_bits=6

for f in search result rebuild bucket_id bucket_dump; do
  [[ -e "$DIR/$f" ]] || { echo "ERROR: missing $DIR/$f"; exit 2; }
done

do_search_and_check() {
  local x="$1"

  echo "$x" | sudo tee "$DIR/search" >/dev/null

  local res
  res="$(cat "$DIR/result")"
  echo "$res"

  # ожидаем строку вида: found=1 value=42 bucket=2
  local found value bucket
  found="$(echo "$res"  | sed -n 's/.*found=\([0-9]\+\).*/\1/p')"
  value="$(echo "$res"  | sed -n 's/.*value=\([0-9]\+\).*/\1/p')"
  bucket="$(echo "$res" | sed -n 's/.*bucket=\([0-9]\+\).*/\1/p')"

  [[ -n "$found" && -n "$value" && -n "$bucket" ]] || {
    echo "ERROR: cannot parse result: $res" >&2
    exit 3
  }

  [[ "$found" == "1" ]] || {
    echo "ERROR: expected found=1 for x=$x, got: $res" >&2
    exit 4
  }

  echo "$bucket" | sudo tee "$DIR/bucket_id" >/dev/null

  local dump
  dump="$(cat "$DIR/bucket_dump")"
  echo "$dump"

  # Проверяем, что значение присутствует в дампе (целое слово)
  echo "$dump" | grep -Eq "(^|[[:space:]:])${x}([[:space:]]|$)" || {
    echo "ERROR: bucket_dump does not contain ${x} (bucket=${bucket})" >&2
    exit 5
  }
}

# 1) Поиск до rebuild
do_search_and_check 42

# 2) rebuild и повтор
echo 1 | sudo tee "$DIR/rebuild" >/dev/null
do_search_and_check 42

sudo rmmod "$MOD"
echo OK
