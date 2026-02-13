#pragma once

typedef struct {
    char **lines;
    int line_count;
    int capacity;

    int cursor_row;
    int cursor_col;
} TextBuffer;

void tb_init(TextBuffer *tb);
void tb_free(TextBuffer *tb);

void tb_insert_char(TextBuffer *tb, int ch);
void tb_backspace(TextBuffer *tb);
void tb_newline(TextBuffer *tb);

void tb_move_left(TextBuffer *tb);
void tb_move_right(TextBuffer *tb);
void tb_move_up(TextBuffer *tb);
void tb_move_down(TextBuffer *tb);

char *tb_to_string(const TextBuffer *tb);

void tb_set_from_string(TextBuffer *tb, const char *s);