#include <ncurses.h>
#include <string.h>
#include "ui/input.h"
#include "core/actions.h"
#include "core/dispatch.h"

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

    tb_insert_char(tb, ch);
}

static void editor_handle_insert_key(AppState *s, int ch) {
    if (s->active_field == EDIT_FIELD_URL) {
        handle_url_insert(s, ch);
        return;
    }

    if (s->active_field == EDIT_FIELD_BODY) {
        handle_textbuf_insert(&s->body, ch);
        return;
    }

    handle_textbuf_insert(&s->headers, ch);
}

void ui_handle_key(AppState *state, Keymap *keymap, int ch) {
    Action a = keymap_resolve(keymap, state->mode, ch);

    if (a != ACT_NONE) {
        dispatch_action(state, a);
        return;
    }

    if (state->mode == MODE_INSERT) {
        editor_handle_insert_key(state, ch);
    }
}
