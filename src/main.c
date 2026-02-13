#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "state.h"
#include "core/keymap.h"
#include "core/actions.h"
#include "core/textbuf.h"
#include <curl/curl.h>

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

static const char *method_label(HttpMethod m) {
    switch (m) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DELETE";
        default: return "?";
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

    int max_items = h - 2;
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

static void draw_response_content(WINDOW *w, const AppState *state) {
    int h, wd;
    getmaxyx(w, h, wd);

    if (state->is_request_in_flight) {
        mvwaddnstr(w, 1, 2, "Sending request...", wd - 4);
        wnoutrefresh(w);
        return;
    }

    if (!state->response.body) {
        mvwaddnstr(w, 1, 2, "No response yet", wd - 4);
        wnoutrefresh(w);
        return;
    }

    char status[64];
    snprintf(status, sizeof(status), "Status: %ld", state->response.status);
    mvwaddnstr(w, 1, 2, status, wd - 4);

    int row = 2;
    const char *p = state->response.body;
    while (*p && row < h - 1) {
        mvwaddnstr(w, row++, 2, p, wd - 4);
        const char *nl = strchr(p, '\n');
        if (!nl) break;
        p = nl + 1;
    }

    wnoutrefresh(w);
}


static void draw_editor_content(WINDOW *w, const AppState *state) {
    int h, wd;
    getmaxyx(w, h, wd);

    // Layout (inside the window box):
    // row 1: URL label
    // row 2: URL value
    // row 3: BODY label
    // row 4: BODY lines
    const int url_label_row = 1;
    const int url_value_row = 2;
    const int body_label_row = 3;
    const int body_first_row = 4;

    // URL label (highlight if active)
    if (state->active_field == EDIT_FIELD_URL) wattron(w, A_REVERSE);
    mvwaddnstr(w, url_label_row, 2, "URL:", wd - 4);
    if (state->active_field == EDIT_FIELD_URL) wattroff(w, A_REVERSE);

    // URL value (clip to available width)
    int url_max = wd - 4;
    if (url_max < 0) url_max = 0;
    mvwaddnstr(w, url_value_row, 2, state->url, url_max);

    // BODY label (highlight if active)
    if (state->active_field == EDIT_FIELD_BODY) wattron(w, A_REVERSE);
    mvwaddnstr(w, body_label_row, 2, "BODY:", wd - 4);
    if (state->active_field == EDIT_FIELD_BODY) wattroff(w, A_REVERSE);

    // BODY lines (clip each line)
    int body_visible_lines = h - 1 - body_first_row; // last usable row is h-2 (since border)
    if (body_visible_lines < 0) body_visible_lines = 0;

    int line_clip = wd - 4;
    if (line_clip < 0) line_clip = 0;

    for (int i = 0; i < body_visible_lines; i++) {
        int row = i;
        if (row >= state->body.line_count) break;

        mvwaddnstr(w, body_first_row + i, 2, state->body.lines[row], line_clip);
    }

    // Cursor handling
    curs_set(0);
    if (state->mode == MODE_INSERT && state->focused_panel == PANEL_EDITOR) {
        if (state->active_field == EDIT_FIELD_URL) {
            int cy = url_value_row;
            int cx = 2 + state->url_cursor;

            if (cx < 2) cx = 2;
            if (cx > wd - 2) cx = wd - 2;

            wmove(w, cy, cx);
            curs_set(1);
        } else {
            int cy = body_first_row + state->body.cursor_row;
            int cx = 2 + state->body.cursor_col;

            if (cy >= body_first_row && cy <= h - 2) {
                if (cx < 2) cx = 2;
                if (cx > wd - 2) cx = wd - 2;

                wmove(w, cy, cx);
                curs_set(1);
            }
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

    char top[256];
    snprintf(top, sizeof(top), " tcurl | %s ", method_label(state->method));
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
            state->active_field == EDIT_FIELD_URL ? " Editor [URL] " : " Editor [BODY] ",
            state->focused_panel == PANEL_EDITOR
        );
        draw_editor_content(w_editor, state);

        draw_boxed_window(
            w_response,
            " Response ",
            state->focused_panel == PANEL_RESPONSE
        );
        draw_response_content(w_response, state);

    }
    

    doupdate();
}

static void editor_handle_insert_key(AppState *s, int ch) {
    if (s->active_field == EDIT_FIELD_URL) {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (s->url_cursor > 0 && s->url_len > 0) {
                memmove(&s->url[s->url_cursor - 1],
                        &s->url[s->url_cursor],
                        (size_t)(s->url_len - s->url_cursor + 1)); // includes '\0'
                s->url_cursor--;
                s->url_len--;
            }
            return;
        }

        if (ch == KEY_LEFT) {
            if (s->url_cursor > 0) s->url_cursor--;
            return;
        }

        if (ch == KEY_RIGHT) {
            if (s->url_cursor < s->url_len) s->url_cursor++;
            return;
        }

        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) return;

        if (ch >= 32 && ch <= 126) {
            if (s->url_len >= URL_MAX - 1) return;

            memmove(&s->url[s->url_cursor + 1],
                    &s->url[s->url_cursor],
                    (size_t)(s->url_len - s->url_cursor + 1)); // shift incl '\0'
            s->url[s->url_cursor] = (char)ch;
            s->url_cursor++;
            s->url_len++;
        }
        return;
    }

    // BODY field (multi-line)
    if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) { tb_backspace(&s->body); return; }
    if (ch == KEY_LEFT)  { tb_move_left(&s->body); return; }
    if (ch == KEY_RIGHT) { tb_move_right(&s->body); return; }
    if (ch == KEY_UP)    { tb_move_up(&s->body); return; }
    if (ch == KEY_DOWN)  { tb_move_down(&s->body); return; }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) { tb_newline(&s->body); return; }

    tb_insert_char(&s->body, ch);
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
    curl_global_init(CURL_GLOBAL_DEFAULT);

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
    app_state_destroy(&state);
    curl_global_cleanup();
    return 0;
}
