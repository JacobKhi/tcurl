#!/bin/sh

if ! command -v pkg-config >/dev/null; then
  echo "pkg-config not found"
  exit 1
fi

if ! pkg-config --exists ncurses; then
  echo "ncurses not found"
  exit 1
fi

echo "Environment OK"
