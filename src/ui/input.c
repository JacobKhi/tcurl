#include <ncurses.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ui/input.h"
#include "core/actions.h"
#include "core/dispatch.h"

static void insert_tab_in_url(AppState *s) {
    const int tab_spaces = 4;
    if (!s) return;
    if (s->url_len >= URL_MAX - 1) return;

    int can_insert = URL_MAX - 1 - s->url_len;
    int n = tab_spaces;
    if (n > can_insert) n = can_insert;
    if (n <= 0) return;

    memmove(
        &s->url[s->url_cursor + n],
        &s->url[s->url_cursor],
        (size_t)(s->url_len - s->url_cursor + 1)
    );
    for (int i = 0; i < n; i++) {
        s->url[s->url_cursor + i] = ' ';
    }
    s->url_cursor += n;
    s->url_len += n;
}

static void handle_url_insert(AppState *s, int ch) {
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (s->url_cursor > 0 && s->url_len > 0) {
            memmove(&s->url[s->url_cursor - 1],
                    &s->url[s->url_cursor],
                    (size_t)(s->url_len - s->url_cursor + 1));
            s->url_cursor--;
            s->url_len--;
        }
        return;
    }

    if (ch == KEY_LEFT)  { if (s->url_cursor > 0) s->url_cursor--; return; }
    if (ch == KEY_RIGHT) { if (s->url_cursor < s->url_len) s->url_cursor++; return; }

    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return;
    if (ch == '\t' || ch == 9) {
        insert_tab_in_url(s);
        return;
    }

    if (ch >= 32 && ch <= 126) {
        if (s->url_len >= URL_MAX - 1) return;

        memmove(&s->url[s->url_cursor + 1],
                &s->url[s->url_cursor],
                (size_t)(s->url_len - s->url_cursor + 1));
        s->url[s->url_cursor] = (char)ch;
        s->url_cursor++;
        s->url_len++;
    }
}

static void handle_textbuf_insert(TextBuffer *tb, int ch) {
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) { tb_backspace(tb); return; }
    if (ch == KEY_LEFT)  { tb_move_left(tb); return; }
    if (ch == KEY_RIGHT) { tb_move_right(tb); return; }
    if (ch == KEY_UP)    { tb_move_up(tb); return; }
    if (ch == KEY_DOWN)  { tb_move_down(tb); return; }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) { tb_newline(tb); return; }
    if (ch == '\t' || ch == 9) {
        tb_insert_char(tb, ' ');
        tb_insert_char(tb, ' ');
        tb_insert_char(tb, ' ');
        tb_insert_char(tb, ' ');
        return;
    }

    tb_insert_char(tb, ch);
}

static void reset_headers_autocomplete(AppState *s) {
    s->headers_ac_row = -1;
    s->headers_ac_next_match = 0;
    free(s->headers_ac_seed);
    s->headers_ac_seed = NULL;
}

static int starts_with_ci(const char *s, const char *prefix) {
    if (!s || !prefix) return 0;

    for (int i = 0; prefix[i]; i++) {
        if (!s[i]) return 0;
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)prefix[i])) return 0;
    }
    return 1;
}

static char *current_header_prefix(const TextBuffer *tb) {
    if (!tb || tb->cursor_row < 0 || tb->cursor_row >= tb->line_count) return NULL;

    const char *line = tb->lines[tb->cursor_row];
    if (!line) return strdup("");

    int len = (int)strlen(line);
    int end = tb->cursor_col;
    if (end < 0) end = 0;
    if (end > len) end = len;

    const char *colon = strchr(line, ':');
    if (colon) {
        int cpos = (int)(colon - line);
        if (cpos < end) end = cpos;
    }

    int start = 0;
    while (start < end && isspace((unsigned char)line[start])) start++;
    while (end > start && isspace((unsigned char)line[end - 1])) end--;

    int n = end - start;
    char *out = malloc((size_t)n + 1);
    if (!out) return NULL;
    memcpy(out, line + start, (size_t)n);
    out[n] = '\0';
    return out;
}

static void headers_autocomplete(AppState *s) {
    if (!s || s->header_suggestions_count <= 0 || !s->header_suggestions) return;

    TextBuffer *tb = &s->headers;
    if (tb->cursor_row < 0 || tb->cursor_row >= tb->line_count) return;

    if (!s->headers_ac_seed || s->headers_ac_row != tb->cursor_row) {
        char *seed = current_header_prefix(tb);
        if (!seed) return;

        reset_headers_autocomplete(s);
        s->headers_ac_seed = seed;
        s->headers_ac_row = tb->cursor_row;
        s->headers_ac_next_match = 0;
    }

    int *matches = malloc((size_t)s->header_suggestions_count * sizeof(*matches));
    if (!matches) return;

    int mcount = 0;
    for (int i = 0; i < s->header_suggestions_count; i++) {
        if (starts_with_ci(s->header_suggestions[i], s->headers_ac_seed)) {
            matches[mcount++] = i;
        }
    }

    if (mcount == 0) {
        free(matches);
        return;
    }

    int pick = matches[s->headers_ac_next_match % mcount];
    s->headers_ac_next_match++;
    free(matches);

    const char *name = s->header_suggestions[pick];
    size_t out_len = strlen(name) + 3;
    char *line = malloc(out_len);
    if (!line) return;
    snprintf(line, out_len, "%s: ", name);

    free(tb->lines[tb->cursor_row]);
    tb->lines[tb->cursor_row] = line;
    tb->cursor_col = (int)strlen(line);
}

static void editor_handle_insert_key(AppState *s, int ch) {
    if (s->active_field == EDIT_FIELD_URL) {
        reset_headers_autocomplete(s);
        handle_url_insert(s, ch);
        return;
    }

    if (s->active_field == EDIT_FIELD_BODY) {
        reset_headers_autocomplete(s);
        handle_textbuf_insert(&s->body, ch);
        return;
    }

    if (ch == KEY_BTAB) {
        headers_autocomplete(s);
        return;
    }

    reset_headers_autocomplete(s);
    handle_textbuf_insert(&s->headers, ch);
}

static void prompt_insert_char(AppState *s, int ch) {
    if (!s) return;

    if (ch >= 32 && ch <= 126) {
        if (s->prompt_len >= PROMPT_MAX - 1) return;

        memmove(
            &s->prompt_input[s->prompt_cursor + 1],
            &s->prompt_input[s->prompt_cursor],
            (size_t)(s->prompt_len - s->prompt_cursor + 1)
        );
        s->prompt_input[s->prompt_cursor] = (char)ch;
        s->prompt_cursor++;
        s->prompt_len++;
    }
}

static void prompt_backspace(AppState *s) {
    if (!s || s->prompt_cursor <= 0 || s->prompt_len <= 0) return;

    memmove(
        &s->prompt_input[s->prompt_cursor - 1],
        &s->prompt_input[s->prompt_cursor],
        (size_t)(s->prompt_len - s->prompt_cursor + 1)
    );
    s->prompt_cursor--;
    s->prompt_len--;
}

static void command_history_push(AppState *s, const char *cmd) {
    if (!s || !cmd || !cmd[0]) return;

    if (s->command_history_count > 0) {
        const char *last = s->command_history[s->command_history_count - 1];
        if (strcmp(last, cmd) == 0) {
            s->command_history_index = s->command_history_count;
            return;
        }
    }

    if (s->command_history_count < CMD_HISTORY_MAX) {
        snprintf(s->command_history[s->command_history_count], PROMPT_MAX, "%s", cmd);
        s->command_history_count++;
    } else {
        memmove(
            s->command_history,
            s->command_history + 1,
            sizeof(s->command_history[0]) * (CMD_HISTORY_MAX - 1)
        );
        snprintf(s->command_history[CMD_HISTORY_MAX - 1], PROMPT_MAX, "%s", cmd);
    }

    s->command_history_index = s->command_history_count;
}

static void command_history_recall(AppState *s) {
    if (!s) return;
    if (s->command_history_index < 0 || s->command_history_index >= s->command_history_count) return;

    strncpy(
        s->prompt_input,
        s->command_history[s->command_history_index],
        PROMPT_MAX - 1
    );
    s->prompt_input[PROMPT_MAX - 1] = '\0';
    s->prompt_len = (int)strlen(s->prompt_input);
    s->prompt_cursor = s->prompt_len;
}

static void handle_command_mode_key(AppState *s, Keymap *km, int ch) {
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        prompt_backspace(s);
        return;
    }

    if (ch == KEY_LEFT) {
        if (s->prompt_cursor > 0) s->prompt_cursor--;
        return;
    }
    if (ch == KEY_RIGHT) {
        if (s->prompt_cursor < s->prompt_len) s->prompt_cursor++;
        return;
    }

    if (ch == KEY_UP) {
        if (s->command_history_count <= 0) return;
        if (s->command_history_index < 0) s->command_history_index = s->command_history_count - 1;
        else if (s->command_history_index > 0) s->command_history_index--;
        command_history_recall(s);
        return;
    }

    if (ch == KEY_DOWN) {
        if (s->command_history_count <= 0) return;
        if (s->command_history_index < 0) return;
        if (s->command_history_index < s->command_history_count - 1) {
            s->command_history_index++;
            command_history_recall(s);
        } else {
            s->command_history_index = -1;
            s->prompt_input[0] = '\0';
            s->prompt_len = 0;
            s->prompt_cursor = 0;
        }
        return;
    }

    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
        command_history_push(s, s->prompt_input);
        dispatch_execute_command(s, km, s->prompt_input);
        return;
    }

    prompt_insert_char(s, ch);
}

static void handle_search_mode_key(AppState *s, int ch) {
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        prompt_backspace(s);
        return;
    }

    if (ch == KEY_LEFT) {
        if (s->prompt_cursor > 0) s->prompt_cursor--;
        return;
    }
    if (ch == KEY_RIGHT) {
        if (s->prompt_cursor < s->prompt_len) s->prompt_cursor++;
        return;
    }

    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
        dispatch_apply_search(s, s->prompt_input);
        s->mode = MODE_NORMAL;
        s->prompt_kind = PROMPT_NONE;
        s->prompt_input[0] = '\0';
        s->prompt_len = 0;
        s->prompt_cursor = 0;
        return;
    }

    prompt_insert_char(s, ch);
}

void ui_handle_key(AppState *state, Keymap *keymap, int ch) {
    app_state_lock(state);

    Action a = keymap_resolve(keymap, state->mode, ch);

    if (a != ACT_NONE) {
        dispatch_action(state, a);
        app_state_unlock(state);
        return;
    }

    if (state->mode == MODE_INSERT) {
        editor_handle_insert_key(state, ch);
        app_state_unlock(state);
        return;
    }

    if (state->mode == MODE_COMMAND) {
        handle_command_mode_key(state, keymap, ch);
        app_state_unlock(state);
        return;
    }

    if (state->mode == MODE_SEARCH) {
        handle_search_mode_key(state, ch);
    }

    app_state_unlock(state);
}
