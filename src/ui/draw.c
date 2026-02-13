#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "core/env.h"
#include "core/history.h"
#include "core/layout.h"
#include "ui/draw.h"

enum {
    PAIR_FOCUS = 1,
    PAIR_TAB_ACTIVE = 2,
    PAIR_GUTTER = 3,
    PAIR_WARN = 4,
    PAIR_ERROR = 5
};

static int g_theme_colors = 0;
static int g_editor_cursor_abs_y = -1;
static int g_editor_cursor_abs_x = -1;

typedef struct {
    int y;
    int x;
    int h;
    int w;
    int ok;
} Rect;

static const char *mode_label(Mode m) {
    switch (m) {
        case MODE_NORMAL:
            return "NORMAL";
        case MODE_INSERT:
            return "INSERT";
        case MODE_COMMAND:
            return "COMMAND";
        case MODE_SEARCH:
            return "SEARCH";
        default:
            return "UNKNOWN";
    }
}

static const char *panel_label(Panel p) {
    switch (p) {
        case PANEL_HISTORY:
            return "HISTORY";
        case PANEL_EDITOR:
            return "EDITOR";
        case PANEL_RESPONSE:
            return "RESPONSE";
        default:
            return "UNKNOWN";
    }
}

static const char *method_label(HttpMethod m) {
    switch (m) {
        case HTTP_GET:
            return "GET";
        case HTTP_POST:
            return "POST";
        case HTTP_PUT:
            return "PUT";
        case HTTP_DELETE:
            return "DELETE";
        default:
            return "?";
    }
}

static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static Rect rect_make(int y, int x, int h, int w) {
    Rect r;
    r.y = y;
    r.x = x;
    r.h = h;
    r.w = w;
    r.ok = (h >= 3 && w >= 10);
    return r;
}

static Rect rect_from_slot(LayoutSlot slot, const Rect slots[4]) {
    switch (slot) {
        case LAYOUT_SLOT_TL:
            return slots[0];
        case LAYOUT_SLOT_TR:
            return slots[1];
        case LAYOUT_SLOT_BL:
            return slots[2];
        case LAYOUT_SLOT_BR:
            return slots[3];
        default:
            return slots[0];
    }
}

static int build_panel_rects(
    const AppState *state,
    int rows,
    int cols,
    Rect *history,
    Rect *editor,
    Rect *response
) {
    if (!state || !history || !editor || !response) return 0;

    int inner_y = 1;
    int inner_x = 1;
    int inner_h = rows - 2;
    int inner_w = cols - 2;

    if (inner_h < 3 || inner_w < 10) return 0;

    if (state->ui_layout_profile == LAYOUT_PROFILE_QUAD) {
        int top_h = (inner_h * state->ui_layout_sizing.quad_split_y_pct) / 100;
        if (inner_h >= 6) {
            top_h = clamp_int(top_h, 3, inner_h - 3);
        }
        int bottom_h = inner_h - top_h;
        int left_w = (inner_w * state->ui_layout_sizing.quad_split_x_pct) / 100;
        if (inner_w >= 20) {
            left_w = clamp_int(left_w, 10, inner_w - 10);
        }
        int right_w = inner_w - left_w;

        Rect slots[4];
        slots[0] = rect_make(inner_y, inner_x, top_h, left_w);
        slots[1] = rect_make(inner_y, inner_x + left_w, top_h, right_w);
        slots[2] = rect_make(inner_y + top_h, inner_x, bottom_h, left_w);
        slots[3] = rect_make(inner_y + top_h, inner_x + left_w, bottom_h, right_w);

        *history = rect_from_slot(state->quad_history_slot, slots);
        *editor = rect_from_slot(state->quad_editor_slot, slots);
        *response = rect_from_slot(state->quad_response_slot, slots);

        return history->ok && editor->ok && response->ok;
    }

    if (state->ui_layout_profile == LAYOUT_PROFILE_FOCUS_EDITOR) {
        int editor_h = (inner_h * state->ui_layout_sizing.focus_editor_height_pct) / 100;
        if (inner_h >= 6) {
            editor_h = clamp_int(editor_h, 3, inner_h - 3);
        }

        int bottom_h = inner_h - editor_h;
        if (editor_h < 3 || bottom_h < 3) return 0;

        int left_w = inner_w / 2;
        if (inner_w >= 20) {
            left_w = clamp_int(left_w, 10, inner_w - 10);
        }
        int right_w = inner_w - left_w;

        *editor = rect_make(inner_y, inner_x, editor_h, inner_w);
        *history = rect_make(inner_y + editor_h, inner_x, bottom_h, left_w);
        *response = rect_make(inner_y + editor_h, inner_x + left_w, bottom_h, right_w);

        return history->ok && editor->ok && response->ok;
    }

    {
        int left_w = (inner_w * state->ui_layout_sizing.classic_history_width_pct) / 100;
        if (inner_w >= 20) {
            left_w = clamp_int(left_w, 10, inner_w - 10);
        }

        int right_w = inner_w - left_w;
        int right_top_h = (inner_h * state->ui_layout_sizing.classic_editor_height_pct) / 100;
        if (inner_h >= 6) {
            right_top_h = clamp_int(right_top_h, 3, inner_h - 3);
        }
        int right_bottom_h = inner_h - right_top_h;

        *history = rect_make(inner_y, inner_x, inner_h, left_w);
        *editor = rect_make(inner_y, inner_x + left_w, right_top_h, right_w);
        *response = rect_make(inner_y + right_top_h, inner_x + left_w, right_bottom_h, right_w);

        return history->ok && editor->ok && response->ok;
    }
}

void ui_draw_init_theme(const AppState *state) {
    g_theme_colors = 0;
    if (!state || !state->ui_theme.use_colors) return;
    if (!has_colors()) return;

    start_color();
    (void)use_default_colors();

    if (init_pair(PAIR_FOCUS, (short)state->ui_theme.focus_fg, (short)state->ui_theme.focus_bg) == ERR) return;
    if (init_pair(PAIR_TAB_ACTIVE, (short)state->ui_theme.tab_active_fg, (short)state->ui_theme.tab_active_bg) == ERR) return;
    if (init_pair(PAIR_GUTTER, (short)state->ui_theme.gutter_fg, (short)state->ui_theme.gutter_bg) == ERR) return;
    if (init_pair(PAIR_WARN, (short)state->ui_theme.warn_fg, (short)state->ui_theme.warn_bg) == ERR) return;
    if (init_pair(PAIR_ERROR, (short)state->ui_theme.error_fg, (short)state->ui_theme.error_bg) == ERR) return;

    g_theme_colors = 1;
}

static void draw_boxed_window(WINDOW *w, const char *title, int focused) {
    int h, wd;
    getmaxyx(w, h, wd);
    (void)h;

    werase(w);

    if (focused) {
        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_FOCUS) | A_BOLD);
        else wattron(w, A_REVERSE);
    }

    box(w, 0, 0);

    if (focused) {
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_FOCUS) | A_BOLD);
        else wattroff(w, A_REVERSE);
    }

    if (title && wd > 4) {
        if (focused) {
            if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_FOCUS) | A_BOLD);
            else wattron(w, A_REVERSE);
        } else {
            wattron(w, A_BOLD);
        }

        mvwaddnstr(w, 0, 2, title, wd - 4);

        if (focused) {
            if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_FOCUS) | A_BOLD);
            else wattroff(w, A_REVERSE);
        } else {
            wattroff(w, A_BOLD);
        }
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

static void draw_history_content(WINDOW *w, const AppState *state) {
    int h, wd;
    getmaxyx(w, h, wd);

    if (!state->history) {
        wnoutrefresh(w);
        return;
    }

    int visible = h - 2;
    if (visible <= 0) {
        wnoutrefresh(w);
        return;
    }

    int start = 0;
    if (state->history_selected >= visible) {
        start = state->history_selected - visible + 1;
    }
    if (start > state->history->count - visible) {
        start = state->history->count - visible;
    }
    if (start < 0) start = 0;

    for (int i = 0; i < visible; i++) {
        int idx = start + i;
        if (idx >= state->history->count) break;

        const HistoryItem *it = &state->history->items[idx];

        if (idx == state->history_selected) {
            if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
            else wattron(w, A_REVERSE);
        }

        char line[1024];
        snprintf(
            line,
            sizeof(line),
            "%s %s",
            (it->method == HTTP_GET) ? "GET" :
            (it->method == HTTP_POST) ? "POST" :
            (it->method == HTTP_PUT) ? "PUT" : "DEL",
            it->url ? it->url : ""
        );
        mvwaddnstr(w, 1 + i, 2, line, wd - 4);

        if (idx == state->history_selected) {
            if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
            else wattroff(w, A_REVERSE);
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

    if (state->response.error) {
        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_ERROR) | A_BOLD);
        else wattron(w, A_BOLD);
        mvwaddnstr(w, 1, 2, "Request failed:", wd - 4);
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_ERROR) | A_BOLD);
        else wattroff(w, A_BOLD);

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
    snprintf(
        meta,
        sizeof(meta),
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
    if (body_start >= h - 1) {
        wnoutrefresh(w);
        return;
    }

    const char *p = skip_lines(state->response.body_view, state->response_scroll);

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

static void keep_scroll_visible(const TextBuffer *tb, int *scroll, int visible, int active) {
    if (!tb || !scroll || visible <= 0) return;

    int max_scroll = tb->line_count - visible;
    if (max_scroll < 0) max_scroll = 0;

    if (active) {
        if (tb->cursor_row < *scroll) {
            *scroll = tb->cursor_row;
        }
        if (tb->cursor_row >= *scroll + visible) {
            *scroll = tb->cursor_row - visible + 1;
        }
    }

    *scroll = clamp_int(*scroll, 0, max_scroll);
}

static void draw_textbuffer_area(
    WINDOW *w,
    const TextBuffer *tb,
    int *scroll,
    int y_first,
    int y_last,
    int x_left,
    int x_right,
    int active,
    int *out_cursor_y,
    int *out_cursor_x
) {
    if (out_cursor_y) *out_cursor_y = -1;
    if (out_cursor_x) *out_cursor_x = -1;

    if (!tb || !scroll) return;

    int visible = y_last - y_first + 1;
    int clip = x_right - x_left + 1;
    if (visible <= 0 || clip <= 0) return;

    keep_scroll_visible(tb, scroll, visible, active);

    int gutter = (clip >= 8) ? 4 : 0;
    int text_left = x_left + gutter;
    int text_clip = x_right - text_left + 1;
    if (text_clip < 0) text_clip = 0;
    int active_hscroll = 0;
    if (active && text_clip > 0 && tb->cursor_col >= text_clip) {
        active_hscroll = tb->cursor_col - text_clip + 1;
    }

    for (int i = 0; i < visible; i++) {
        int y = y_first + i;
        int row = *scroll + i;

        if (active && row == tb->cursor_row) {
            if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
            else wattron(w, A_REVERSE);
            mvwhline(w, y, x_left, ' ', clip);
            if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
            else wattroff(w, A_REVERSE);
        }

        if (gutter > 0 && row < tb->line_count) {
            char ln[16];
            snprintf(ln, sizeof(ln), "%3d ", row + 1);
            if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_GUTTER));
            mvwaddnstr(w, y, x_left, ln, gutter);
            if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_GUTTER));
        }

        if (row < tb->line_count && text_clip > 0) {
            const char *line = tb->lines[row];
            int start_col = 0;
            if (active && row == tb->cursor_row) start_col = active_hscroll;

            int line_len = (int)strlen(line);
            if (start_col > line_len) start_col = line_len;

            mvwaddnstr(w, y, text_left, line + start_col, text_clip);
        }

        if (active && row == tb->cursor_row && out_cursor_y && out_cursor_x) {
            int cx = text_left + (tb->cursor_col - active_hscroll);
            if (cx < text_left) cx = text_left;
            if (cx > x_right) cx = x_right;
            *out_cursor_y = y;
            *out_cursor_x = cx;
        }
    }
}

static void draw_editor_split_content(WINDOW *w, AppState *state, int *out_cy, int *out_cx) {
    int h, wd;
    getmaxyx(w, h, wd);

    const int url_label_row = 1;
    const int url_value_row = 2;
    const int split_label_row = 3;
    const int content_first_row = 4;
    const int content_last_row = h - 2;

    if (state->active_field == EDIT_FIELD_URL) {
        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattron(w, A_REVERSE);
    }
    mvwaddnstr(w, url_label_row, 2, "URL:", wd - 4);
    if (state->active_field == EDIT_FIELD_URL) {
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattroff(w, A_REVERSE);
    }

    int url_clip = wd - 4;
    if (url_clip < 0) url_clip = 0;
    int url_hscroll = 0;
    if (url_clip > 0 && state->url_cursor >= url_clip) {
        url_hscroll = state->url_cursor - url_clip + 1;
    }

    if (state->active_field == EDIT_FIELD_URL) {
        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattron(w, A_REVERSE);
        mvwhline(w, url_value_row, 2, ' ', url_clip);
    }
    mvwaddnstr(w, url_value_row, 2, state->url + url_hscroll, url_clip);
    if (state->active_field == EDIT_FIELD_URL) {
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattroff(w, A_REVERSE);
    }

    if (content_first_row > content_last_row || wd < 12) {
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

    if (state->active_field == EDIT_FIELD_BODY) {
        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattron(w, A_REVERSE);
    }
    mvwaddnstr(w, split_label_row, body_left, "BODY", body_right - body_left + 1);
    if (state->active_field == EDIT_FIELD_BODY) {
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattroff(w, A_REVERSE);
    }

    if (state->active_field == EDIT_FIELD_HEADERS) {
        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattron(w, A_REVERSE);
    }
    mvwaddnstr(w, split_label_row, headers_left, "HEADERS", headers_right - headers_left + 1);
    if (state->active_field == EDIT_FIELD_HEADERS) {
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattroff(w, A_REVERSE);
    }

    int cy = -1;
    int cx = -1;

    draw_textbuffer_area(
        w,
        &state->body,
        &state->body_scroll,
        content_first_row,
        content_last_row,
        body_left,
        body_right,
        state->active_field == EDIT_FIELD_BODY,
        state->active_field == EDIT_FIELD_BODY ? &cy : NULL,
        state->active_field == EDIT_FIELD_BODY ? &cx : NULL
    );

    draw_textbuffer_area(
        w,
        &state->headers,
        &state->headers_scroll,
        content_first_row,
        content_last_row,
        headers_left,
        headers_right,
        state->active_field == EDIT_FIELD_HEADERS,
        state->active_field == EDIT_FIELD_HEADERS ? &cy : NULL,
        state->active_field == EDIT_FIELD_HEADERS ? &cx : NULL
    );

    if (state->active_field == EDIT_FIELD_URL) {
        cy = url_value_row;
        cx = 2 + (state->url_cursor - url_hscroll);
        if (cx < 2) cx = 2;
        if (cx > wd - 2) cx = wd - 2;
    }

    if (out_cy) *out_cy = cy;
    if (out_cx) *out_cx = cx;
}

static void draw_editor_tab_item(WINDOW *w, int y, int x, const char *label, int active) {
    char txt[32];
    snprintf(txt, sizeof(txt), " %s ", label);

    if (active) {
        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE) | A_BOLD);
        else wattron(w, A_REVERSE);
    } else {
        wattron(w, A_BOLD);
    }

    mvwaddstr(w, y, x, txt);

    if (active) {
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE) | A_BOLD);
        else wattroff(w, A_REVERSE);
    } else {
        wattroff(w, A_BOLD);
    }
}

static void draw_editor_tabs_content(WINDOW *w, AppState *state, int *out_cy, int *out_cx) {
    int h, wd;
    getmaxyx(w, h, wd);

    int x = 2;
    draw_editor_tab_item(w, 1, x, "URL", state->active_field == EDIT_FIELD_URL);
    x += 7;
    draw_editor_tab_item(w, 1, x, "BODY", state->active_field == EDIT_FIELD_BODY);
    x += 8;
    draw_editor_tab_item(w, 1, x, "HEADERS", state->active_field == EDIT_FIELD_HEADERS);

    mvwhline(w, 2, 1, ACS_HLINE, wd - 2);

    int content_first = 3;
    int content_last = h - 2;
    if (content_first > content_last) return;

    int cy = -1;
    int cx = -1;

    if (state->active_field == EDIT_FIELD_URL) {
        int line_y = content_first;
        int clip = wd - 4;
        if (clip < 0) clip = 0;
        int url_hscroll = 0;
        if (clip > 0 && state->url_cursor >= clip) {
            url_hscroll = state->url_cursor - clip + 1;
        }

        if (g_theme_colors) wattron(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattron(w, A_REVERSE);
        mvwhline(w, line_y, 2, ' ', clip);
        mvwaddnstr(w, line_y, 2, state->url + url_hscroll, clip);
        if (g_theme_colors) wattroff(w, COLOR_PAIR(PAIR_TAB_ACTIVE));
        else wattroff(w, A_REVERSE);

        cy = line_y;
        cx = 2 + (state->url_cursor - url_hscroll);
        if (cx < 2) cx = 2;
        if (cx > wd - 2) cx = wd - 2;
    } else if (state->active_field == EDIT_FIELD_BODY) {
        draw_textbuffer_area(
            w,
            &state->body,
            &state->body_scroll,
            content_first,
            content_last,
            2,
            wd - 3,
            1,
            &cy,
            &cx
        );
    } else {
        draw_textbuffer_area(
            w,
            &state->headers,
            &state->headers_scroll,
            content_first,
            content_last,
            2,
            wd - 3,
            1,
            &cy,
            &cx
        );
    }

    if (out_cy) *out_cy = cy;
    if (out_cx) *out_cx = cx;
}

static void draw_editor_content(WINDOW *w, AppState *state) {
    int cursor_y = -1;
    int cursor_x = -1;

    if (state->ui_layout_profile == LAYOUT_PROFILE_FOCUS_EDITOR) {
        draw_editor_tabs_content(w, state, &cursor_y, &cursor_x);
    } else {
        draw_editor_split_content(w, state, &cursor_y, &cursor_x);
    }

    if (state->mode == MODE_INSERT &&
        state->focused_panel == PANEL_EDITOR &&
        cursor_y >= 0 &&
        cursor_x >= 0) {
        int wy, wx;
        getbegyx(w, wy, wx);
        g_editor_cursor_abs_y = wy + cursor_y;
        g_editor_cursor_abs_x = wx + cursor_x;
    }

    wnoutrefresh(w);
}

void ui_draw(AppState *state) {
    static WINDOW *w_history = NULL;
    static WINDOW *w_editor = NULL;
    static WINDOW *w_response = NULL;

    static int last_rows = -1;
    static int last_cols = -1;
    static LayoutProfile last_profile = -1;
    static LayoutSlot last_h_slot = -1;
    static LayoutSlot last_e_slot = -1;
    static LayoutSlot last_r_slot = -1;

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    app_state_lock(state);
    g_editor_cursor_abs_y = -1;
    g_editor_cursor_abs_x = -1;

    int layout_changed =
        rows != last_rows ||
        cols != last_cols ||
        state->ui_layout_profile != last_profile ||
        state->quad_history_slot != last_h_slot ||
        state->quad_editor_slot != last_e_slot ||
        state->quad_response_slot != last_r_slot;

    if (layout_changed) {
        if (w_history) {
            delwin(w_history);
            w_history = NULL;
        }
        if (w_editor) {
            delwin(w_editor);
            w_editor = NULL;
        }
        if (w_response) {
            delwin(w_response);
            w_response = NULL;
        }

        last_rows = rows;
        last_cols = cols;
        last_profile = state->ui_layout_profile;
        last_h_slot = state->quad_history_slot;
        last_e_slot = state->quad_editor_slot;
        last_r_slot = state->quad_response_slot;

        Rect rh = {0};
        Rect re = {0};
        Rect rr = {0};
        if (build_panel_rects(state, rows, cols, &rh, &re, &rr)) {
            w_history = newwin(rh.h, rh.w, rh.y, rh.x);
            w_editor = newwin(re.h, re.w, re.y, re.x);
            w_response = newwin(rr.h, rr.w, rr.y, rr.x);

            if (w_history) leaveok(w_history, TRUE);
            if (w_editor) leaveok(w_editor, FALSE);
            if (w_response) leaveok(w_response, TRUE);
        }
    }

    erase();
    box(stdscr, 0, 0);

    char top[256];
    snprintf(
        top,
        sizeof(top),
        " tcurl | %s | layout=%s ",
        method_label(state->method),
        layout_profile_name(state->ui_layout_profile)
    );
    if (g_theme_colors) attron(COLOR_PAIR(PAIR_FOCUS) | A_BOLD);
    else attron(A_BOLD);
    mvaddnstr(0, 2, top, cols - 4);
    if (g_theme_colors) attroff(COLOR_PAIR(PAIR_FOCUS) | A_BOLD);
    else attroff(A_BOLD);

    if (state->mode == MODE_COMMAND || state->mode == MODE_SEARCH) {
        char prompt[512];
        char prefix = (state->mode == MODE_COMMAND) ? ':' : '/';
        snprintf(prompt, sizeof(prompt), " %c%s", prefix, state->prompt_input);
        mvaddnstr(rows - 1, 2, prompt, cols - 4);
    } else {
        char status[512];
        int status_warn = 0;
        const char *env_name = env_store_active_name(&state->envs);
        int status_x = 2;
        int status_limit_x = cols - 2;

        if (state->search_not_found && state->search_query[0] != '\0') {
            snprintf(
                status,
                sizeof(status),
                " %s | focus=%s | env=%s | history_selected=%d | load_skipped=%d | not found: %s ",
                mode_label(state->mode),
                panel_label(state->focused_panel),
                env_name ? env_name : "none",
                state->history_selected,
                state->history_skipped_invalid,
                state->search_query
            );
            status_warn = 1;
        } else {
            snprintf(
                status,
                sizeof(status),
                " %s | focus=%s | env=%s | history_selected=%d | load_skipped=%d | save_err=%d ",
                mode_label(state->mode),
                panel_label(state->focused_panel),
                env_name ? env_name : "none",
                state->history_selected,
                state->history_skipped_invalid,
                state->history_last_save_error
            );
        }

        if (state->ui_show_footer_hint && cols > 20) {
            const char *hint = ":h help  :q quit  Move: h/j/k/l or arrows";
            int hint_len = (int)strlen(hint);
            int max_hint_len = cols - 8;
            if (hint_len > max_hint_len) hint_len = max_hint_len;

            int hint_x = (cols - 2) - hint_len;
            int sep_x = hint_x - 3;
            if (sep_x > status_x + 10) {
                status_limit_x = sep_x;
                mvaddnstr(rows - 1, sep_x, " | ", cols - sep_x - 2);
                attron(A_DIM);
                mvaddnstr(rows - 1, hint_x, hint, hint_len);
                attroff(A_DIM);
            }
        }

        if (status_x < status_limit_x) {
            int status_space = status_limit_x - status_x;
            if (status_warn && g_theme_colors) attron(COLOR_PAIR(PAIR_WARN) | A_BOLD);
            mvaddnstr(rows - 1, status_x, status, status_space);
            if (status_warn && g_theme_colors) attroff(COLOR_PAIR(PAIR_WARN) | A_BOLD);
        }
    }

    wnoutrefresh(stdscr);

    if (w_history && w_editor && w_response) {
        draw_boxed_window(w_history, " History ", state->focused_panel == PANEL_HISTORY);
        draw_history_content(w_history, state);

        draw_boxed_window(
            w_editor,
            state->ui_layout_profile == LAYOUT_PROFILE_FOCUS_EDITOR
                ? " Editor [TABS] "
                : (state->active_field == EDIT_FIELD_URL
                       ? " Editor [URL] "
                       : (state->active_field == EDIT_FIELD_BODY
                              ? " Editor [BODY] "
                              : " Editor [HEADERS] ")),
            state->focused_panel == PANEL_EDITOR
        );
        draw_editor_content(w_editor, state);

        draw_boxed_window(w_response, " Response ", state->focused_panel == PANEL_RESPONSE);
        draw_response_content(w_response, state);
    }

    if (state->mode == MODE_COMMAND || state->mode == MODE_SEARCH) {
        int cx = 4 + state->prompt_cursor;
        if (cx < 2) cx = 2;
        if (cx > cols - 2) cx = cols - 2;
        move(rows - 1, cx);
        curs_set(1);
    } else if (state->mode == MODE_INSERT &&
               state->focused_panel == PANEL_EDITOR &&
               g_editor_cursor_abs_y >= 0 &&
               g_editor_cursor_abs_x >= 0) {
        move(g_editor_cursor_abs_y, g_editor_cursor_abs_x);
        curs_set(1);
    } else {
        curs_set(0);
    }

    app_state_unlock(state);
    doupdate();
}
