#include "core/textbuf.h"
#include <stdlib.h>
#include <string.h>

static char *dup_str(const char *s) {
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static int ensure_capacity(TextBuffer *tb, int need) {
    if (tb->capacity >= need) return 1;
    int new_cap = tb->capacity ? tb->capacity : 4;
    while (new_cap < need) new_cap *= 2;

    char **nl = (char **)realloc(tb->lines, (size_t)new_cap * sizeof(char *));
    if (!nl) return 0;

    tb->lines = nl;
    tb->capacity = new_cap;
    return 1;
}

void tb_init(TextBuffer *tb) {
    tb->lines = NULL;
    tb->line_count = 0;
    tb->capacity = 0;
    tb->cursor_row = 0;
    tb->cursor_col = 0;

    ensure_capacity(tb, 1);
    tb->lines[0] = dup_str("");
    tb->line_count = 1;
}

void tb_free(TextBuffer *tb) {
    if (!tb) return;
    for (int i = 0; i < tb->line_count; i++) free(tb->lines[i]);
    free(tb->lines);
    tb->lines = NULL;
    tb->line_count = 0;
    tb->capacity = 0;
    tb->cursor_row = 0;
    tb->cursor_col = 0;
}

static int line_len(const TextBuffer *tb, int row) {
    if (row < 0 || row >= tb->line_count) return 0;
    return (int)strlen(tb->lines[row]);
}

void tb_insert_char(TextBuffer *tb, int ch) {
    if (!tb) return;
    if (ch < 32 || ch > 126) return;

    char *line = tb->lines[tb->cursor_row];
    int len = (int)strlen(line);

    if (tb->cursor_col < 0) tb->cursor_col = 0;
    if (tb->cursor_col > len) tb->cursor_col = len;

    char *nl = (char *)realloc(line, (size_t)len + 2);
    if (!nl) return;

    memmove(&nl[tb->cursor_col + 1], &nl[tb->cursor_col], (size_t)(len - tb->cursor_col + 1));
    nl[tb->cursor_col] = (char)ch;

    tb->lines[tb->cursor_row] = nl;
    tb->cursor_col++;
}

void tb_backspace(TextBuffer *tb) {
    if (!tb) return;

    if (tb->cursor_col > 0) {
        char *line = tb->lines[tb->cursor_row];
        int len = (int)strlen(line);

        memmove(&line[tb->cursor_col - 1], &line[tb->cursor_col], (size_t)(len - tb->cursor_col + 1));
        tb->cursor_col--;

        char *nl = (char *)realloc(line, (size_t)len);
        if (nl) tb->lines[tb->cursor_row] = nl;
        return;
    }

    if (tb->cursor_row > 0) {
        int cur = tb->cursor_row;
        int prev = cur - 1;

        char *prev_line = tb->lines[prev];
        char *cur_line  = tb->lines[cur];

        int prev_len = (int)strlen(prev_line);
        int cur_len  = (int)strlen(cur_line);

        char *merged = (char *)realloc(prev_line, (size_t)(prev_len + cur_len + 1));
        if (!merged) return;

        memcpy(merged + prev_len, cur_line, (size_t)cur_len + 1);
        tb->lines[prev] = merged;

        free(cur_line);

        for (int i = cur; i < tb->line_count - 1; i++) tb->lines[i] = tb->lines[i + 1];
        tb->line_count--;

        tb->cursor_row = prev;
        tb->cursor_col = prev_len;
    }
}

void tb_newline(TextBuffer *tb) {
    if (!tb) return;

    int row = tb->cursor_row;
    char *line = tb->lines[row];
    int len = (int)strlen(line);

    if (tb->cursor_col < 0) tb->cursor_col = 0;
    if (tb->cursor_col > len) tb->cursor_col = len;

    char *right = dup_str(line + tb->cursor_col);
    if (!right) return;

    line[tb->cursor_col] = '\0';
    char *left = (char *)realloc(line, (size_t)tb->cursor_col + 1);
    if (left) tb->lines[row] = left;

    if (!ensure_capacity(tb, tb->line_count + 1)) {
        return;
    }

    for (int i = tb->line_count; i > row + 1; i--) tb->lines[i] = tb->lines[i - 1];
    tb->lines[row + 1] = right;
    tb->line_count++;

    tb->cursor_row++;
    tb->cursor_col = 0;
}

void tb_move_left(TextBuffer *tb) {
    if (!tb) return;
    if (tb->cursor_col > 0) tb->cursor_col--;
    else if (tb->cursor_row > 0) {
        tb->cursor_row--;
        tb->cursor_col = line_len(tb, tb->cursor_row);
    }
}

void tb_move_right(TextBuffer *tb) {
    if (!tb) return;
    int len = line_len(tb, tb->cursor_row);
    if (tb->cursor_col < len) tb->cursor_col++;
    else if (tb->cursor_row < tb->line_count - 1) {
        tb->cursor_row++;
        tb->cursor_col = 0;
    }
}

void tb_move_up(TextBuffer *tb) {
    if (!tb) return;
    if (tb->cursor_row > 0) {
        tb->cursor_row--;
        int len = line_len(tb, tb->cursor_row);
        if (tb->cursor_col > len) tb->cursor_col = len;
    }
}

void tb_move_down(TextBuffer *tb) {
    if (!tb) return;
    if (tb->cursor_row < tb->line_count - 1) {
        tb->cursor_row++;
        int len = line_len(tb, tb->cursor_row);
        if (tb->cursor_col > len) tb->cursor_col = len;
    }
}
