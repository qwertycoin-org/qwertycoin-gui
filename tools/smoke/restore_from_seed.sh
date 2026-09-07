#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <path-to-qwertycoin-wallet-cli> [work-dir]" >&2
  exit 64
fi

wallet_cli=$1
work_dir=${2:-"$(mktemp -d)"}
password="qwc-smoke-password"

if [[ ! -x "$wallet_cli" ]]; then
  echo "wallet CLI is not executable: $wallet_cli" >&2
  exit 66
fi

mkdir -p "$work_dir"
work_dir=$(cd "$work_dir" && pwd)

first_wallet="$work_dir/restored-wallet-1"
second_wallet="$work_dir/restored-wallet-2"
first_log="$work_dir/restored-1.log"
second_log="$work_dir/restored-2.log"
test_seed="amaze buffet cake entrance symptoms tiger lamb maze nestle python dusted faxed update vague zinger boxes ornament renting glass gained island nabbing afield calamity boxes"

rm -f "$first_wallet" "$first_wallet".* "$second_wallet" "$second_wallet".*

printf 'address\nexit\n' | "$wallet_cli" \
  --restore-deterministic-wallet \
  --generate-new-wallet "$first_wallet" \
  --password "$password" \
  --electrum-seed "$test_seed" \
  --restore-height 0 \
  --offline \
  >"$first_log" 2>&1

first_address=$(grep -Eo 'QWC[[:alnum:]]+' "$first_log" | head -n 1 || true)

if [[ -z "$first_address" || "$first_address" != QWC* ]]; then
  echo "failed to extract restored QWC address from $first_log" >&2
  exit 1
fi

printf 'address\nexit\n' | "$wallet_cli" \
  --restore-deterministic-wallet \
  --generate-new-wallet "$second_wallet" \
  --password "$password" \
  --electrum-seed "$test_seed" \
  --restore-height 0 \
  --offline \
  >"$second_log" 2>&1

second_address=$(grep -Eo 'QWC[[:alnum:]]+' "$second_log" | head -n 1 || true)

if [[ "$second_address" != "$first_address" ]]; then
  echo "restored addresses do not match" >&2
  echo "first:  $first_address" >&2
  echo "second: $second_address" >&2
  echo "first log:  $first_log" >&2
  echo "second log: $second_log" >&2
  exit 1
fi

echo "restore-from-seed smoke passed"
echo "address=$first_address"
echo "work_dir=$work_dir"
