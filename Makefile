CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude
LDFLAGS ?= -lncurses -lcurl -lpthread -lcjson

TARGET = tcurl
SRC = \
  src/main.c \
  src/state.c \
  src/ui/draw.c \
  src/ui/input.c \
  src/core/actions.c \
  src/core/keymap.c \
  src/core/dispatch.c \
  src/core/paths.c \
  src/core/textbuf.c \
  src/core/history.c \
  src/core/history_storage.c \
  src/core/layout.c \
  src/core/env.c \
  src/core/http.c \
  src/core/request_thread.c \
  src/core/format.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

deps:
	sh scripts/setup.sh

check-deps:
	@missing=0; \
	for cmd in gcc make pkg-config; do \
		if ! command -v $$cmd >/dev/null 2>&1; then \
			echo "Missing required command: $$cmd"; \
			missing=1; \
		fi; \
	done; \
	for pc in ncurses libcurl libcjson; do \
		if ! pkg-config --exists $$pc; then \
			echo "Missing pkg-config dependency: $$pc"; \
			missing=1; \
		fi; \
	done; \
	if [ $$missing -ne 0 ]; then \
		echo "Dependency check failed."; \
		exit 1; \
	fi; \
	echo "All dependencies are available."

install-user: $(TARGET)
	@set -e; \
	BIN_DIR="$$HOME/.local/bin"; \
	CONF_DIR="$$HOME/.config/tcurl"; \
	mkdir -p "$$BIN_DIR" "$$CONF_DIR"; \
	cp "$(TARGET)" "$$BIN_DIR/tcurl"; \
	chmod 755 "$$BIN_DIR/tcurl"; \
	for f in keymap.conf layout.conf themes.conf envs.json headers.txt history.conf; do \
		if [ ! -f "$$CONF_DIR/$$f" ]; then \
			cp "config/$$f" "$$CONF_DIR/$$f"; \
		fi; \
	done; \
	echo "Installed $$BIN_DIR/tcurl"; \
	echo "Config directory: $$CONF_DIR"; \
	case ":$$PATH:" in \
		*":$$BIN_DIR:"*) \
			echo "PATH check: $$BIN_DIR is already available." ;; \
		*) \
			echo "WARNING: $$BIN_DIR is not in PATH."; \
			echo "Add this to your shell config (e.g. ~/.zshrc):"; \
			echo '  export PATH="$$HOME/.local/bin:$$PATH"'; \
			echo "Then restart your shell."; ;; \
	esac

uninstall-user:
	@set -e; \
	BIN_PATH="$$HOME/.local/bin/tcurl"; \
	if [ -f "$$BIN_PATH" ]; then \
		rm -f "$$BIN_PATH"; \
		echo "Removed $$BIN_PATH"; \
	else \
		echo "$$BIN_PATH not found."; \
	fi; \
	echo "User config kept at $$HOME/.config/tcurl"

.PHONY: all clean deps check-deps install-user uninstall-user
