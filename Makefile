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
  src/core/auth.c \
  src/core/keymap.c \
  src/core/dispatch.c \
  src/core/export.c \
  src/core/paths.c \
  src/core/request_snapshot.c \
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
	rm -f tests/run_tests tests/run_tests_asan

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

TEST_TARGET = tests/run_tests
TEST_SRC = \
  tests/test_main.c \
  tests/test_keymap.c \
  tests/test_layout.c \
  tests/test_env.c \
  tests/test_history_storage.c \
  tests/test_format.c \
  tests/test_export_auth.c
TEST_CORE_SRC = \
  src/core/actions.c \
  src/core/auth.c \
  src/core/export.c \
  src/core/keymap.c \
  src/core/layout.c \
  src/core/env.c \
  src/core/textbuf.c \
  src/core/history.c \
  src/core/history_storage.c \
  src/core/format.c
TEST_LDFLAGS = -lncurses -lcjson -lpthread

$(TEST_TARGET): $(TEST_SRC) $(TEST_CORE_SRC)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(TEST_CORE_SRC) $(TEST_LDFLAGS)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

test-sanitizers:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -o tests/run_tests_asan $(TEST_SRC) $(TEST_CORE_SRC) $(TEST_LDFLAGS) -fsanitize=address,undefined
	ASAN_OPTIONS=detect_leaks=0 ./tests/run_tests_asan

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

release: $(TARGET)
	sh scripts/release.sh

.PHONY: all clean deps check-deps test test-sanitizers install-user uninstall-user release
