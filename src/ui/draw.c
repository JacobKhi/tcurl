#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include "core/env.h"
#include "core/history.h"
#include "ui/draw.h"

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
    int h, wd;
    getmaxyx(w, h, wd);
    (void)wd;

    if (!state->history) {
        wnoutrefresh(w);
        return;
    }

    int max = h - 2;
    for (int i = 0; i < max && i < state->history->count; i++) {
        const HistoryItem *it = &state->history->items[i];

        if (i == state->history_selected)
            wattron(w, A_REVERSE);

        mvwprintw(
            w, 1 + i, 2,
            "%s %s",
            (it->method == HTTP_GET) ? "GET" :
            (it->method == HTTP_POST) ? "POST" :
            (it->method == HTTP_PUT) ? "PUT" : "DEL",
            it->url ? it->url : ""
        );

        if (i == state->history_selected)
            wattroff(w, A_REVERSE);
    }

    wnoutrefresh(w);
}

static const char *skip_lines(const char *s, int n) {
    const char *p = s;
    while (p && *p && n > 0) {
        const char *nl = strchr(p, '\n');
        if (!nl) return p + strlen(p);
        p = nl + 1;
        n--;
    }
    return p;
}

static void draw_response_content(WINDOW *w, const AppState *state) {
    int h, wd;
    getmaxyx(w, h, wd);

    if (state->is_request_in_flight) {
        mvwaddnstr(w, 1, 2, "Sending request...", wd - 4);
        wnoutrefresh(w);
        return;
    }

    if (state->response.error) {
        wattron(w, A_BOLD);
        mvwaddnstr(w, 1, 2, "Request failed:", wd - 4);
        wattroff(w, A_BOLD);

        mvwaddnstr(w, 2, 2, state->response.error, wd - 4);
        wnoutrefresh(w);
        return;
    }

    if (!state->response.body_view) {
        mvwaddnstr(w, 1, 2, "No response yet", wd - 4);
        wnoutrefresh(w);
        return;
    }

    size_t bytes = strlen(state->response.body_view);

    char meta[256];
    snprintf(meta, sizeof(meta),
        "Status: %ld | Time: %.0f ms | Size: %.1f KB%s | scroll:%d",
        state->response.status,
        state->response.elapsed_ms,
        bytes / 1024.0,
        state->response.is_json ? " | JSON" : "",
        state->response_scroll
    );

    mvwaddnstr(w, 1, 2, meta, wd - 4);

    mvwhline(w, 2, 1, ACS_HLINE, wd - 2);

    int body_start = 3;
    int visible = h - body_start - 1;
    if (visible <= 0) {
        wnoutrefresh(w);
        return;
    }

    const char *p = skip_lines(
        state->response.body_view,
        state->response_scroll
    );

    int row = body_start;
    int clip = wd - 4;
    if (clip < 0) clip = 0;

    while (*p && row < h - 1) {
        const char *nl = strchr(p, '\n');
        if (!nl) {
            mvwaddnstr(w, row, 2, p, clip);
            break;
        }

        int len = (int)(nl - p);
        if (len > clip) len = clip;

        mvwaddnstr(w, row, 2, p, len);
        p = nl + 1;
        row++;
    }

    wnoutrefresh(w);
}


static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static void draw_editor_content(WINDOW *w, AppState *state) {
    int h, wd;
    getmaxyx(w, h, wd);

    const int url_label_row = 1;
    const int url_value_row = 2;
    const int split_label_row = 3;
    const int content_first_row = 4;
    const int content_last_row = h - 2;

    if (state->active_field == EDIT_FIELD_URL) wattron(w, A_REVERSE);
    mvwaddnstr(w, url_label_row, 2, "URL:", wd - 4);
    if (state->active_field == EDIT_FIELD_URL) wattroff(w, A_REVERSE);

    int url_max = wd - 4;
    if (url_max < 0) url_max = 0;
    mvwaddnstr(w, url_value_row, 2, state->url, url_max);

    if (content_first_row > content_last_row || wd < 12) {
        wnoutrefresh(w);
        return;
    }

    int left_x = 2;
    int right_x = wd - 3;
    int divider_x = wd / 2;
    if (divider_x <= left_x) divider_x = left_x + 1;
    if (divider_x >= right_x) divider_x = right_x - 1;

    int body_left = left_x;
    int body_right = divider_x - 2;
    int headers_left = divider_x + 1;
    int headers_right = right_x;

    if (body_right < body_left) body_right = body_left;
    if (headers_right < headers_left) headers_right = headers_left;

    int divider_len = h - split_label_row - 1;
    if (divider_len > 0) {
        mvwvline(w, split_label_row, divider_x, ACS_VLINE, divider_len);
    }

    if (state->active_field == EDIT_FIELD_BODY) wattron(w, A_REVERSE);
    mvwaddnstr(w, split_label_row, body_left, "BODY", body_right - body_left + 1);
    if (state->active_field == EDIT_FIELD_BODY) wattroff(w, A_REVERSE);

    if (state->active_field == EDIT_FIELD_HEADERS) wattron(w, A_REVERSE);
    mvwaddnstr(w, split_label_row, headers_left, "HEADERS", headers_right - headers_left + 1);
    if (state->active_field == EDIT_FIELD_HEADERS) wattroff(w, A_REVERSE);

    int body_clip = body_right - body_left + 1;
    if (body_clip < 0) body_clip = 0;
    int headers_clip = headers_right - headers_left + 1;
    if (headers_clip < 0) headers_clip = 0;

    int body_gutter = body_clip >= 8 ? 4 : 0;
    int headers_gutter = headers_clip >= 8 ? 4 : 0;

    int body_text_left = body_left + body_gutter;
    int headers_text_left = headers_left + headers_gutter;
    int body_text_clip = body_right - body_text_left + 1;
    int headers_text_clip = headers_right - headers_text_left + 1;
    if (body_text_clip < 0) body_text_clip = 0;
    if (headers_text_clip < 0) headers_text_clip = 0;

    int visible_lines = content_last_row - content_first_row + 1;
    if (visible_lines <= 0) {
        wnoutrefresh(w);
        return;
    }

    int max_body_scroll = state->body.line_count - visible_lines;
    if (max_body_scroll < 0) max_body_scroll = 0;
    int max_headers_scroll = state->headers.line_count - visible_lines;
    if (max_headers_scroll < 0) max_headers_scroll = 0;

    if (state->active_field == EDIT_FIELD_BODY) {
        if (state->body.cursor_row < state->body_scroll) {
            state->body_scroll = state->body.cursor_row;
        }
        if (state->body.cursor_row >= state->body_scroll + visible_lines) {
            state->body_scroll = state->body.cursor_row - visible_lines + 1;
        }
    } else if (state->active_field == EDIT_FIELD_HEADERS) {
        if (state->headers.cursor_row < state->headers_scroll) {
            state->headers_scroll = state->headers.cursor_row;
        }
        if (state->headers.cursor_row >= state->headers_scroll + visible_lines) {
            state->headers_scroll = state->headers.cursor_row - visible_lines + 1;
        }
    }

    state->body_scroll = clamp_int(state->body_scroll, 0, max_body_scroll);
    state->headers_scroll = clamp_int(state->headers_scroll, 0, max_headers_scroll);

    for (int i = 0; i < visible_lines; i++) {
        int row_y = content_first_row + i;
        int body_row = state->body_scroll + i;
        int headers_row = state->headers_scroll + i;

        if (state->active_field == EDIT_FIELD_BODY && body_row == state->body.cursor_row) {
            wattron(w, A_REVERSE);
            mvwhline(w, row_y, body_left, ' ', body_clip);
            wattroff(w, A_REVERSE);
        }
        if (state->active_field == EDIT_FIELD_HEADERS && headers_row == state->headers.cursor_row) {
            wattron(w, A_REVERSE);
            mvwhline(w, row_y, headers_left, ' ', headers_clip);
            wattroff(w, A_REVERSE);
        }

        if (body_gutter > 0 && body_row < state->body.line_count) {
            char ln[16];
            snprintf(ln, sizeof(ln), "%3d ", body_row + 1);
            mvwaddnstr(w, row_y, body_left, ln, body_gutter);
        }
        if (headers_gutter > 0 && headers_row < state->headers.line_count) {
            char ln[16];
            snprintf(ln, sizeof(ln), "%3d ", headers_row + 1);
            mvwaddnstr(w, row_y, headers_left, ln, headers_gutter);
        }

        if (body_row < state->body.line_count) {
            mvwaddnstr(w, row_y, body_text_left, state->body.lines[body_row], body_text_clip);
        }
        if (headers_row < state->headers.line_count) {
            mvwaddnstr(w, row_y, headers_text_left, state->headers.lines[headers_row], headers_text_clip);
        }
    }

    curs_set(0);
    if (state->mode == MODE_INSERT && state->focused_panel == PANEL_EDITOR) {
        if (state->active_field == EDIT_FIELD_URL) {
            int cy = url_value_row;
            int cx = 2 + state->url_cursor;

            if (cx < 2) cx = 2;
            if (cx > wd - 2) cx = wd - 2;

            wmove(w, cy, cx);
            curs_set(1);
        } else if (state->active_field == EDIT_FIELD_BODY) {
            int cy = content_first_row + (state->body.cursor_row - state->body_scroll);
            int cx = body_text_left + state->body.cursor_col;

            if (cy >= content_first_row && cy <= content_last_row) {
                if (cx < body_text_left) cx = body_text_left;
                if (cx > body_right) cx = body_right;

                wmove(w, cy, cx);
                curs_set(1);
            }
        } else {
            int cy = content_first_row + (state->headers.cursor_row - state->headers_scroll);
            int cx = headers_text_left + state->headers.cursor_col;

            if (cy >= content_first_row && cy <= content_last_row) {
                if (cx < headers_text_left) cx = headers_text_left;
                if (cx > headers_right) cx = headers_right;

                wmove(w, cy, cx);
                curs_set(1);
            }
        }
    }

    wnoutrefresh(w);
}               

void ui_draw(AppState *state) {
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


    if (state->mode == MODE_COMMAND || state->mode == MODE_SEARCH) {
        char prompt[512];
        char prefix = state->mode == MODE_COMMAND ? ':' : '/';
        snprintf(prompt, sizeof(prompt), " %c%s", prefix, state->prompt_input);
        mvaddnstr(rows - 1, 2, prompt, cols - 4);
    } else {
        char status[512];
        const char *env_name = env_store_active_name(&state->envs);
        if (state->search_not_found && state->search_query[0] != '\0') {
            snprintf(status, sizeof(status), " %s | focus=%s | env=%s | history_selected=%d | not found: %s ",
                     mode_label(state->mode),
                     panel_label(state->focused_panel),
                     env_name ? env_name : "none",
                     state->history_selected,
                     state->search_query);
        } else {
            snprintf(status, sizeof(status), " %s | focus=%s | env=%s | history_selected=%d ",
                     mode_label(state->mode),
                     panel_label(state->focused_panel),
                     env_name ? env_name : "none",
                     state->history_selected);
        }
        mvaddnstr(rows - 1, 2, status, cols - 4);
    }

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
            state->active_field == EDIT_FIELD_URL ? " Editor [URL] " :
            (state->active_field == EDIT_FIELD_BODY ? " Editor [BODY] " : " Editor [HEADERS] "),
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
    if (state->mode == MODE_COMMAND || state->mode == MODE_SEARCH) {
        int cx = 4 + state->prompt_cursor;
        if (cx < 2) cx = 2;
        if (cx > cols - 2) cx = cols - 2;
        move(rows - 1, cx);
        curs_set(1);
    }

    doupdate();
}
