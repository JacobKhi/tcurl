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
  src/core/textbuf.c \
  src/core/history.c \
  src/core/history_storage.c \
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

.PHONY: all clean deps check-deps
