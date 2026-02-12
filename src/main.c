#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "state.h"
#include "core/keymap.h"
#include "core/actions.h"

void dispatch_action(AppState *s, Action a);

static const char *mode_label(Mode m) {
    switch (m) {
        case MODE_NORMAL:  return "NORMAL";
        case MODE_INSERT:  return "INSERT";
        case MODE_COMMAND: return "COMMAND";
        case MODE_SEARCH:  return "SEARCH";
        default:           return "UNKNOWN";
    }
}

static const char *panel_label(Panel p) {
    switch (p) {
        case PANEL_HISTORY:  return "HISTORY";
        case PANEL_EDITOR:   return "EDITOR";
        case PANEL_RESPONSE: return "RESPONSE";
        default:             return "UNKNOWN";
    }
}

static void draw_boxed_window(WINDOW *w, const char *title, int focused) {
    int h, wd;
    getmaxyx(w, h, wd);
    (void)h;

    werase(w);

    if (focused) wattron(w, A_REVERSE);
    box(w, 0, 0);
    if (focused) wattroff(w, A_REVERSE);

    if (title && wd > 4) {
        if (focused) wattron(w, A_REVERSE);
        mvwaddnstr(w, 0, 2, title, wd - 4);
        if (focused) wattroff(w, A_REVERSE);
    }

    wnoutrefresh(w);
}

static void draw_history_content(WINDOW *w, const AppState *state) {
    int h, wdt;
    getmaxyx(w, h, wdt);
    (void)wdt;

    int max_items = h - 2; // inside the box
    if (max_items <= 0) return;

    for (int i = 0; i < max_items; i++) {
        int idx = i; // fake items: Item 0..N

        if (idx == state->history_selected) {
            wattron(w, A_REVERSE);
            mvwprintw(w, 1 + i, 2, "Item %d", idx);
            wattroff(w, A_REVERSE);
        } else {
            mvwprintw(w, 1 + i, 2, "Item %d", idx);
        }
    }

    wnoutrefresh(w);
}

static void draw_editor_content(WINDOW *w, const AppState *state) {
    int h, wd;
    getmaxyx(w, h, wd);

    (void)h;

    mvwaddnstr(w, 1, 2, "URL:", wd - 4);
    // draw url (clipped)
    mvwaddnstr(w, 2, 2, state->url, wd - 4);

    // show cursor only in insert mode and when editor focused
    if (state->mode == MODE_INSERT && state->focused_panel == PANEL_EDITOR) {
        // cursor row 2, col 2 + url_cursor
        int cx = 2 + state->url_cursor;
        int cy = 2;

        // clamp inside window
        if (cx < wd - 1) {
            wmove(w, cy, cx);
            curs_set(1);
        } else {
            curs_set(0);
        }
    }
    wnoutrefresh(w);
}

static void draw_ui(const AppState *state) {
    static WINDOW *w_history  = NULL;
    static WINDOW *w_editor   = NULL;
    static WINDOW *w_response = NULL;

    static int last_rows = -1;
    static int last_cols = -1;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (rows != last_rows || cols != last_cols) {
        if (w_history)  { delwin(w_history);  w_history = NULL; }
        if (w_editor)   { delwin(w_editor);   w_editor = NULL; }
        if (w_response) { delwin(w_response); w_response = NULL; }

        last_rows = rows;
        last_cols = cols;

        int inner_y = 1;
        int inner_h = rows - 2;
        int inner_x = 1;
        int inner_w = cols - 2;

        if (inner_h < 3 || inner_w < 10) {
        } else {
            int left_w = inner_w / 3;
            if (left_w < 20) left_w = 20;
            if (left_w > inner_w - 20) left_w = inner_w - 20;

            int right_w = inner_w - left_w;

            int left_x = inner_x;
            int right_x = inner_x + left_w;

            int right_top_h = inner_h / 2;
            int right_bottom_h = inner_h - right_top_h;

            w_history  = newwin(inner_h, left_w, inner_y, left_x);
            w_editor   = newwin(right_top_h, right_w, inner_y, right_x);
            w_response = newwin(right_bottom_h, right_w, inner_y + right_top_h, right_x);

            leaveok(w_history, TRUE);
            leaveok(w_editor, TRUE);
            leaveok(w_response, TRUE);
        }
    }

    erase();
    box(stdscr, 0, 0);

    const char *top = " tcurl ";
    mvaddnstr(0, 2, top, cols - 4);

    char status[256];
    snprintf(status, sizeof(status), " %s | focus=%s | history_selected=%d ",
             mode_label(state->mode),
             panel_label(state->focused_panel),
             state->history_selected);
    mvaddnstr(rows - 1, 2, status, cols - 4);

    wnoutrefresh(stdscr);

    if (w_history && w_editor && w_response) {
        draw_boxed_window(
            w_history,
            " History ",
            state->focused_panel == PANEL_HISTORY
        );
        draw_history_content(w_history, state);

        draw_boxed_window(
            w_editor,
            " Editor ",
            state->focused_panel == PANEL_EDITOR
        );
        draw_editor_content(w_editor, state);

        draw_boxed_window(
            w_response,
            " Response ",
            state->focused_panel == PANEL_RESPONSE
        );

    }
    

    doupdate();
}

static void editor_handle_insert_key(AppState *s, int ch) {
    // Backspace (varia por terminal)
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
        if (s->url_cursor > 0 && s->url_len > 0) {
            // remove char before cursor
            memmove(&s->url[s->url_cursor - 1],
                    &s->url[s->url_cursor],
                    (size_t)(s->url_len - s->url_cursor + 1)); // +1 includes '\0'
            s->url_cursor--;
            s->url_len--;
        }
        return;
    }

    // Left/Right arrows
    if (ch == KEY_LEFT) {
        if (s->url_cursor > 0) s->url_cursor--;
        return;
    }
    if (ch == KEY_RIGHT) {
        if (s->url_cursor < s->url_len) s->url_cursor++;
        return;
    }

    // Printable ASCII (keep it simple for MVP)
    if (ch >= 32 && ch <= 126) {
        if (s->url_len >= URL_MAX - 1) return;

        // insert at cursor
        memmove(&s->url[s->url_cursor + 1],
                &s->url[s->url_cursor],
                (size_t)(s->url_len - s->url_cursor + 1)); // shift incl '\0'
        s->url[s->url_cursor] = (char)ch;
        s->url_cursor++;
        s->url_len++;
    }
}

int main(void) {
    setlocale(LC_ALL, "");

    AppState state;
    app_state_init(&state);

    Keymap keymap;
    (void)keymap_load_file(&keymap, "config/keymap.conf");

    initscr();
    set_escdelay(25);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    while (state.running) {
        draw_ui(&state);

        int ch = getch();
        if (ch == ERR) continue;

        Action a = keymap_resolve(&keymap, state.mode, ch);

        if (a != ACT_NONE) {
            dispatch_action(&state, a);
        } else if (state.mode == MODE_INSERT) {
            editor_handle_insert_key(&state, ch);
        }
    }

    endwin();
    return 0;
}
