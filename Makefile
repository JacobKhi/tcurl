CC ?= gcc

# Package names vary for cJSON across distros/toolchains.
CJSON_PKG := $(shell if pkg-config --exists libcjson; then echo libcjson; else echo cjson; fi)
PKG_CFLAGS := $(shell pkg-config --cflags ncurses libcurl $(CJSON_PKG) 2>/dev/null)
PKG_LIBS := $(shell pkg-config --libs ncurses libcurl $(CJSON_PKG) 2>/dev/null)

CFLAGS ?= -Wall -Wextra -O2 -Iinclude $(PKG_CFLAGS)
LDFLAGS ?= $(PKG_LIBS) -lpthread

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
  src/core/format.c \
  src/core/i18n.c \
  src/core/utils.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
	rm -f tests/run_tests tests/run_tests_asan

deps:
	sh scripts/setup.sh

check-deps:
	sh scripts/check-deps.sh

TEST_TARGET = tests/run_tests
TEST_SRC = \
  tests/test_main.c \
  tests/test_keymap.c \
  tests/test_layout.c \
  tests/test_env.c \
  tests/test_history_storage.c \
  tests/test_format.c \
  tests/test_export_auth.c \
  tests/test_i18n.c \
  tests/test_utils.c \
  tests/test_textbuf_navigation.c
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
  src/core/format.c \
  src/core/i18n.c \
  src/core/utils.c
TEST_LDFLAGS = $(PKG_LIBS) -lpthread

$(TEST_TARGET): $(TEST_SRC) $(TEST_CORE_SRC)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_SRC) $(TEST_CORE_SRC) $(TEST_LDFLAGS)

test: $(TEST_TARGET)
	sh scripts/test.sh $(TEST_TARGET)

test-sanitizers:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -o tests/run_tests_asan $(TEST_SRC) $(TEST_CORE_SRC) $(TEST_LDFLAGS) -fsanitize=address,undefined
	sh scripts/test-sanitizers.sh tests/run_tests_asan

install-user: $(TARGET)
	sh scripts/install-user.sh $(TARGET)

uninstall-user:
	sh scripts/uninstall-user.sh

release: $(TARGET)
	sh scripts/release.sh

.PHONY: all clean deps check-deps test test-sanitizers install-user uninstall-user release
