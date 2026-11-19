#!/usr/bin/env bash
# Regression tests for ymm.
#  1. runs every tests/*.ymm and compares its output (and exit code) with <name>.expected
#  2. compiles it with -c (.ymc) and -c -h (.yb) and checks both give the same result as the source
#     (only the error excerpt differs: compiled files have no source text to quote)
# A test whose first line contains "~ no-compile" must instead be rejected by -c.
#
# usage: tests/run.sh [path/to/ymm] [--update]   (--update rewrites the .expected files)
set -u
here="$(cd "$(dirname "$0")" && pwd)"
ymm="${1:-$here/../build/release/ymm}"
[ -f "$ymm" ] || ymm="$ymm.exe"
[ -x "$ymm" ] || { echo "ymm binary not found: $ymm"; exit 1; }
ymm="$(cd "$(dirname "$ymm")" && pwd)/$(basename "$ymm")"
update=0
[ "${2:-}" = "--update" ] && update=1

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cp "$here"/*.ymm "$work"/
cd "$work"

# stable text: no timings, colors, CR or file extensions
esc="$(printf '\033')" cr="$(printf '\r')" # literal chars: BSD sed (macOS) has no \x1b, \r or \b
norm() { sed -E "s/in [0-9.e+-]+ seconds/in X seconds/; s/${esc}\[[0-9;]*m//g; s/${cr}\$//; s/\.(ymm|ymc|yb)([^A-Za-z0-9_]|\$)/.Y\2/g"; }
# drop traceback excerpts ("Traceback ... col: N]" + source line + caret), keep everything else
no_trace() { awk '/^Traceback/ { if ($0 ~ /col:/) skip = 2; next } skip > 0 { skip--; next } { print }'; }
to=""; command -v timeout >/dev/null && to="timeout 60" # macOS has no timeout by default
run() { $to "$ymm" "$@" </dev/null 2>&1; echo "rc=$?"; }

pass=0 fail=0
for f in *.ymm; do
	name="${f%.ymm}"
	out="$(run "$f" | norm)"
	if [ $update = 1 ]; then
		printf '%s\n' "$out" > "$here/$name.expected"
	elif [ "$out" != "$(cat "$here/$name.expected")" ]; then
		echo "FAIL $name: output differs from $name.expected"
		diff <(printf '%s\n' "$out") "$here/$name.expected" | head -8
		fail=$((fail + 1)); continue
	fi

	if head -1 "$f" | grep -q '~ no-compile'; then
		if "$ymm" -c "$f" >/dev/null 2>&1; then
			echo "FAIL $name: -c should reject this program"; fail=$((fail + 1)); continue
		fi
		pass=$((pass + 1)); continue
	fi

	ok=1
	for mode in ymc yb; do
		flags="-c"; [ $mode = yb ] && flags="-c -h"
		if ! "$ymm" $flags "$f" >/dev/null 2>&1; then
			echo "FAIL $name: ymm $flags failed"; ok=0; break
		fi
		compiled="$(run "$name.$mode" | norm | no_trace)"
		if [ "$compiled" != "$(printf '%s\n' "$out" | no_trace)" ]; then
			echo "FAIL $name: .$mode output differs from source"
			diff <(printf '%s\n' "$compiled") <(printf '%s\n' "$out" | no_trace) | head -8
			ok=0; break
		fi
	done
	if [ $ok = 1 ]; then pass=$((pass + 1)); else fail=$((fail + 1)); fi
done

[ $update = 1 ] && echo "updated expected output for $pass tests"
echo "$pass passed, $fail failed"
[ $fail = 0 ]
