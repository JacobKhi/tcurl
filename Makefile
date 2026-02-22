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
  src/ui/panels/draw.c \
  src/ui/input/input.c \
  src/core/interaction/actions.c \
  src/core/interaction/auth.c \
  src/core/config/keymap.c \
  src/orchestration/dispatch.c \
  src/core/format/export.c \
  src/core/storage/paths.c \
  src/core/http/request_snapshot.c \
  src/core/text/textbuf.c \
  src/core/storage/history.c \
  src/core/storage/history_persistence.c \
  src/core/config/layout.c \
  src/core/config/env.c \
  src/core/http/http.c \
  src/core/http/request_thread.c \
  src/core/format/format.c \
  src/core/text/i18n.c \
  src/core/utils/utils.c \
  src/core/interaction/search.c \
  src/core/cli/command_handlers.c \
  src/core/cli/help_builder.c \
  src/core/cli/command_parser.c

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
  tests/test_textbuf_navigation.c \
  tests/test_search.c \
  tests/test_command_handlers.c \
  tests/test_help_builder.c \
  tests/test_request_snapshot.c \
  tests/test_actions.c \
  tests/test_dispatch.c
TEST_CORE_SRC = \
  src/state.c \
  src/core/interaction/actions.c \
  src/core/interaction/auth.c \
  src/core/format/export.c \
  src/core/config/keymap.c \
  src/core/config/layout.c \
  src/core/config/env.c \
  src/core/storage/paths.c \
  src/core/text/textbuf.c \
  src/core/storage/history.c \
  src/core/storage/history_persistence.c \
  src/core/format/format.c \
  src/core/text/i18n.c \
  src/core/utils/utils.c \
  src/core/interaction/search.c \
  src/core/http/request_snapshot.c \
  src/core/cli/command_handlers.c \
  src/core/cli/help_builder.c \
  src/core/cli/command_parser.c \
  src/ui/panels/draw.c \
  src/orchestration/dispatch.c \
  src/core/http/request_thread.c \
  src/core/http/http.c
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
