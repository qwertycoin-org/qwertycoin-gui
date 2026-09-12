#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 <artifact-directory> <maximum-glibc-version>" >&2
  exit 64
fi

artifact_dir=$(realpath "$1")
maximum_glibc=$2

if [[ ! -d "$artifact_dir" ]]; then
  echo "artifact directory does not exist: $artifact_dir" >&2
  exit 1
fi

if [[ ! "$maximum_glibc" =~ ^[0-9]+([.][0-9]+)+$ ]]; then
  echo "invalid maximum glibc version: $maximum_glibc" >&2
  exit 64
fi

for command_name in file readelf realpath sort; do
  if ! command -v "$command_name" >/dev/null 2>&1; then
    echo "missing Linux ABI verification command: $command_name" >&2
    exit 1
  fi
done

version_is_greater() {
  local candidate=$1
  local maximum=$2
  local highest
  highest=$(printf '%s\n%s\n' "$candidate" "$maximum" | LC_ALL=C sort -Vu | tail -n 1)
  [[ "$candidate" != "$maximum" && "$highest" == "$candidate" ]]
}

violations=""
observed_versions=""
elf_count=0

while IFS= read -r -d '' candidate; do
  [[ "$(file -Lb "$candidate")" == ELF* ]] || continue
  elf_count=$((elf_count + 1))

  while IFS= read -r required_version; do
    [[ -n "$required_version" ]] || continue
    observed_versions+="$required_version"$'\n'
    if version_is_greater "$required_version" "$maximum_glibc"; then
      violations+="${candidate#"$artifact_dir/"}: GLIBC_$required_version"$'\n'
    fi
  done < <(
    readelf --version-info "$candidate" 2>/dev/null \
      | grep -Eo 'GLIBC_[0-9]+([.][0-9]+)+' \
      | sed 's/^GLIBC_//' \
      | LC_ALL=C sort -Vu \
      || true
  )
done < <(find "$artifact_dir" -type f -print0)

if [[ "$elf_count" -eq 0 || -z "$observed_versions" ]]; then
  echo "no versioned Linux ELF files found in $artifact_dir" >&2
  exit 1
fi

if [[ -n "$violations" ]]; then
  echo "Linux artifact exceeds the GLIBC_$maximum_glibc compatibility ceiling:" >&2
  printf '%s' "$violations" | LC_ALL=C sort -u >&2
  exit 1
fi

highest_observed=$(printf '%s' "$observed_versions" | LC_ALL=C sort -Vu | tail -n 1)
echo "Linux ABI verified: $elf_count ELF files, maximum GLIBC_$highest_observed (ceiling GLIBC_$maximum_glibc)"
