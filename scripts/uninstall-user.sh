#!/bin/sh

set -eu

BIN_PATH="$HOME/.local/bin/tcurl"

if [ -f "$BIN_PATH" ]; then
  rm -f "$BIN_PATH"
  echo "Removed $BIN_PATH"
else
  echo "$BIN_PATH not found."
fi

echo "User config kept at $HOME/.config/tcurl"
