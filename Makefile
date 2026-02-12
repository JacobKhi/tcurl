CC = gcc
CFLAGS = -Wall -Wextra -O2 -Iinclude
LDFLAGS = -lncurses

TARGET = tcurl
SRC = \
  src/main.c \
  src/state.c \
  src/core/actions.c \
  src/core/keymap.c \
  src/core/dispatch.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
