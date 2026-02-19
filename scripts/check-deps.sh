#!/bin/sh

set -eu

missing=0

for cmd in gcc make pkg-config; do
  if ! command -v "$cmd" >/dev/null 2>&1; then
    echo "Missing required command: $cmd"
    missing=1
  fi
done

for pc in ncurses libcurl; do
  if ! pkg-config --exists "$pc"; then
    echo "Missing pkg-config dependency: $pc"
    missing=1
  fi
done

if ! pkg-config --exists libcjson && ! pkg-config --exists cjson; then
  echo "Missing pkg-config dependency: libcjson (or cjson)"
  missing=1
fi

if [ "$missing" -ne 0 ]; then
  echo "Dependency check failed."
  exit 1
fi

echo "All dependencies are available."
