#!/bin/sh

set -eu

if [ $# -lt 1 ]; then
  echo "Usage: $0 <target_binary>"
  exit 1
fi

TARGET="$1"

if [ ! -f "$TARGET" ]; then
  echo "Error: Binary $TARGET not found."
  echo "Run 'make' first to build the project."
  exit 1
fi

BIN_DIR="$HOME/.local/bin"
CONF_DIR="$HOME/.config/tcurl"

mkdir -p "$BIN_DIR" "$CONF_DIR"

cp "$TARGET" "$BIN_DIR/tcurl"
chmod 755 "$BIN_DIR/tcurl"

for f in keymap.conf layout.conf themes.conf envs.json headers.txt history.conf; do
  if [ ! -f "$CONF_DIR/$f" ]; then
    cp "config/$f" "$CONF_DIR/$f"
  fi
done

echo "Installed $BIN_DIR/tcurl"
echo "Config directory: $CONF_DIR"

case ":$PATH:" in
  *":$BIN_DIR:"*)
    echo "PATH check: $BIN_DIR is already available." ;;
  *)
    echo "WARNING: $BIN_DIR is not in PATH."
    echo "Add this to your shell config (e.g. ~/.zshrc):"
    echo '  export PATH="$HOME/.local/bin:$PATH"'
    echo "Then restart your shell." ;;
esac
