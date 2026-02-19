#!/bin/sh

set -eu

if [ $# -lt 1 ]; then
  echo "Usage: $0 <test_binary>"
  exit 1
fi

TEST_BINARY="$1"

if [ ! -f "$TEST_BINARY" ]; then
  echo "Error: Test binary $TEST_BINARY not found."
  exit 1
fi

echo "Running tests..."
"./$TEST_BINARY"
