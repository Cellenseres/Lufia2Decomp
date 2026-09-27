#!/bin/sh
# Report clang-format differences on lines changed since a base commit.
# Usage: scripts/check_format.sh [base]   (default: HEAD~1)
# Only changed C sources and headers are inspected; untouched historical code
# is never reformatted. Exits 1 when a changed line differs from .clang-format.
set -eu

base=${1:-}
case "$base" in
    "" | 0000000000000000000000000000000000000000) base=HEAD~1 ;;
esac
if ! git rev-parse --verify --quiet "$base^{commit}" >/dev/null; then
    echo "check_format: base $base is not available; nothing checked"
    exit 0
fi

out=$(git clang-format --diff --extensions c,h "$base" -- src include || true)
case "$out" in
    "" | "no modified files to format"* | "clang-format did not modify any files"*)
        echo "check_format: changed lines match .clang-format"
        exit 0
        ;;
esac
printf '%s\n' "$out"
echo "check_format: changed lines differ from .clang-format" >&2
exit 1
