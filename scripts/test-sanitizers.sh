#!/bin/sh

set -eu

if [ $# -lt 1 ]; then
  echo "Usage: $0 <test_binary_asan>"
  exit 1
fi

TEST_BINARY_ASAN="$1"

if [ ! -f "$TEST_BINARY_ASAN" ]; then
  echo "Error: Test binary $TEST_BINARY_ASAN not found."
  exit 1
fi

echo "Running tests with AddressSanitizer..."
ASAN_OPTIONS=detect_leaks=0 "./$TEST_BINARY_ASAN"
