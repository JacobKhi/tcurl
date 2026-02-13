CC ?= gcc
CFLAGS ?= -Wall -Wextra -O2 -Iinclude
LDFLAGS ?= -lncurses -lcurl -lpthread -lcjson

TARGET = tcurl
SRC = \
  src/main.c \
  src/state.c \
  src/core/actions.c \
  src/core/keymap.c \
  src/core/dispatch.c \
  src/core/textbuf.c \
  src/core/http.c \
  src/core/request_thread.c \
  src/core/format.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
