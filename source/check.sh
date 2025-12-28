#!/usr/bin/env bash
set -euo pipefail

MODNAME="HW_02_hello_world"
KO="./HW_02_hello_world.ko"
PHRASE='Hello, World!'
PARAM_DIR="/sys/module/${MODNAME}/parameters"

cleanup() {
  sudo rmmod "$MODNAME" >/dev/null 2>&1 || true
}
trap cleanup EXIT

if [[ ! -f "$KO" ]]; then
  echo "ERROR: $KO not found. Run 'make' first." >&2
  exit 1
fi

# выгрузить если уже загружен
sudo rmmod "$MODNAME" >/dev/null 2>&1 || true

# очистить dmesg (может быть запрещено, не критично)
sudo dmesg -C >/dev/null 2>&1 || true

sudo insmod "$KO"

# проверить параметры
for p in idx ch_val my_str; do
  if [[ ! -e "${PARAM_DIR}/${p}" ]]; then
    echo "ERROR: missing parameter ${PARAM_DIR}/${p}" >&2
    exit 2
  fi
done

# my_str должен быть RO (запись должна упасть)
set +e
printf "X\n" | sudo tee "${PARAM_DIR}/my_str" >/dev/null 2>&1
rc=$?
set -e
if [[ $rc -eq 0 ]]; then
  echo "ERROR: my_str is writable, but must be read-only" >&2
  exit 3
fi

wparam() {
  local file="$1"
  local value="$2"
  printf "%s\n" "$value" | sudo tee "$file" >/dev/null
}

# собрать строку через idx + ch_val
for ((i=0; i<${#PHRASE}; i++)); do
  ch="${PHRASE:i:1}"
  code="$(printf '%d' "'$ch")"
  wparam "${PARAM_DIR}/idx" "$i"
  wparam "${PARAM_DIR}/ch_val" "$code"
done

got="$(cat "${PARAM_DIR}/my_str" | tr -d '\n')"
if [[ "$got" != "$PHRASE" ]]; then
  echo "ERROR: my_str mismatch"
  echo "  got: '$got'"
  echo "  exp: '$PHRASE'"
  exit 4
fi

sudo rmmod "$MODNAME"

dmesg_out="$(sudo dmesg || true)"
echo "$dmesg_out" | grep -Fq "${MODNAME}: init" || { echo "ERROR: missing '${MODNAME}: init' in dmesg"; exit 5; }
echo "$dmesg_out" | grep -Fq "${MODNAME}: exit" || { echo "ERROR: missing '${MODNAME}: exit' in dmesg"; exit 6; }

echo "OK"

sudo dmesg | tail -n 30
