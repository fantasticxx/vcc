#!/usr/bin/env zsh

set -u -x

readonly VCC="$PWD/vcc"

function try() {
	local expected="$1"
	local input="$2"

	local tmp_in="$(mktemp).vc"
	local tmp_exe="$(mktemp)"

	echo "$input" > "$tmp_in"
	"$VCC" -o "$tmp_exe" "$tmp_in"
	chmod +x $tmp_exe
	"$tmp_exe"
	local actual="$?"

	if [ "$actual" = "$expected" ];	then
		echo "$input => $actual"
	else
		echo "$input => $expected expected, but got $actual"
		echo "input: $tmp_in"
		echo "executable: $tmp_exe"
		exit 1
	fi
}


function try_() {
	local expected="$1"
	local input="$(cat)"
	try "$expected" "$input"
}

function items() {
	local expected="$1"
	local input="$2"
	try "$expected" "void main() { $input }"
}

function expr() {
	local expected="$1"
	local input="$2"
	items "$expected" "exit($input);"
}

expr 0 0
expr 42 42

expr 42 "24 + 18"
expr 30 "58 - 28"
expr 10 "5 * 2"

echo "All tests passed"