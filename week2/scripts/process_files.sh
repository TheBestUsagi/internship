#!/usr/bin/env bash
set -Eeuo pipefail

if [ $# -ne 1 ]; then
  echo "usage: bash scripts/process_files.sh <keyword>"
  exit 1
fi

keyword="$1"
log="result.log"
root="$(pwd)"
: > "$log"   # clear result.org

# iteration from current root .txt
while IFS= read -r -d '' f; do
  if grep -q -- "$keyword" "$f"; then
    if command -v realpath >/dev/null 2>&1; then
      apath="$(realpath "$f")"
    else
      apath="$root/${f#./}"
    fi
    echo "$apath" >> "$log"
    chmod 444 "$f" || true
  fi
done < <(find . -type f -name "*.txt" ! -name "$log" -print0)

count="$(wc -l < "$log" | tr -d ' ')"
echo "${count} files found，the permisssion is changed to read-only！"

