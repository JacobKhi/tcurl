#include "state.h"
#include "core/actions.h"
#include "core/history.h"
#include "core/history_storage.h"
#include <pthread.h>
#include "core/request_thread.h"
#include "core/dispatch.h"
#include "core/env.h"
#include "core/layout.h"
#include "ui/draw.h"
#include <ncurses.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KEY_SENTER
#define KEY_SENTER 548
#endif

static void start_request_if_possible(AppState *s) {
    if (s->is_request_in_flight) return;

    s->is_request_in_flight = 1;
    pthread_t t;
    pthread_create(&t, NULL, request_thread, s);
    pthread_detach(t);
}

static int load_history_item_into_state(AppState *s, const HistoryItem *it) {
    if (!s || !it) return 0;

    s->method = it->method;

    strncpy(s->url, it->url ? it->url : "", sizeof(s->url) - 1);
    s->url[sizeof(s->url) - 1] = '\0';
    s->url_len = strlen(s->url);
    s->url_cursor = s->url_len;

    tb_set_from_string(&s->body, it->body);
    tb_set_from_string(&s->headers, it->headers);

    free(s->response.body);
    free(s->response.body_view);
    free(s->response.error);

    s->response.status = it->status;
    s->response.elapsed_ms = it->elapsed_ms;
    s->response.is_json = it->is_json;
    s->response.body = it->response_body ? strdup(it->response_body) : NULL;
    s->response.body_view = it->response_body_view ? strdup(it->response_body_view) : NULL;
    s->response.error = NULL;
    s->response_scroll = 0;
    s->body_scroll = 0;
    s->headers_scroll = 0;

    return 1;
}

static void clear_prompt(AppState *s) {
    s->prompt_kind = PROMPT_NONE;
    s->prompt_input[0] = '\0';
    s->prompt_len = 0;
    s->prompt_cursor = 0;
}

static void begin_prompt(AppState *s, PromptKind kind) {
    s->prompt_kind = kind;
    s->prompt_input[0] = '\0';
    s->prompt_len = 0;
    s->prompt_cursor = 0;
}

static int contains_ci_n(const char *hay, size_t hay_n, const char *needle) {
    if (!needle || !needle[0]) return 1;
    size_t n = strlen(needle);
    if (n > hay_n) return 0;

    for (size_t i = 0; i + n <= hay_n; i++) {
        size_t j = 0;
        while (j < n) {
            unsigned char a = (unsigned char)hay[i + j];
            unsigned char b = (unsigned char)needle[j];
            if (tolower(a) != tolower(b)) break;
            j++;
        }
        if (j == n) return 1;
    }
    return 0;
}

static const char *method_short(int method) {
    switch (method) {
        case HTTP_GET: return "GET";
        case HTTP_POST: return "POST";
        case HTTP_PUT: return "PUT";
        case HTTP_DELETE: return "DEL";
        default: return "?";
    }
}

static int history_item_matches(const HistoryItem *it, const char *query) {
    char line[2048];
    snprintf(line, sizeof(line), "%s %s", method_short(it->method), it->url ? it->url : "");
    return contains_ci_n(line, strlen(line), query);
}

static int *collect_history_matches(const AppState *s, const char *query, int *out_count) {
    *out_count = 0;
    if (!s->history || s->history->count <= 0 || !query || !*query) return NULL;

    int *matches = malloc((size_t)s->history->count * sizeof(*matches));
    if (!matches) return NULL;

    int mcount = 0;
    for (int i = 0; i < s->history->count; i++) {
        if (history_item_matches(&s->history->items[i], query)) {
            matches[mcount++] = i;
        }
    }

    if (mcount == 0) {
        free(matches);
        return NULL;
    }

    *out_count = mcount;
    return matches;
}

static int *collect_response_matches(const char *body, const char *query, int *out_count) {
    *out_count = 0;
    if (!body || !query || !*query) return NULL;

    int cap = 16;
    int count = 0;
    int *lines = malloc((size_t)cap * sizeof(*lines));
    if (!lines) return NULL;

    int line_no = 0;
    const char *p = body;
    while (1) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);

        if (contains_ci_n(p, len, query)) {
            if (count == cap) {
                int new_cap = cap * 2;
                int *n = realloc(lines, (size_t)new_cap * sizeof(*n));
                if (!n) {
                    free(lines);
                    return NULL;
                }
                lines = n;
                cap = new_cap;
            }
            lines[count++] = line_no;
        }

        if (!nl) break;
        p = nl + 1;
        line_no++;
    }

    if (count == 0) {
        free(lines);
        return NULL;
    }

    *out_count = count;
    return lines;
}

static int pick_match_with_wrap(const int *matches, int count, int current, int dir) {
    if (!matches || count <= 0) return -1;

    if (current < 0) return dir >= 0 ? matches[0] : matches[count - 1];

    if (dir >= 0) {
        for (int i = 0; i < count; i++) {
            if (matches[i] > current) return matches[i];
        }
        return matches[0];
    }

    for (int i = count - 1; i >= 0; i--) {
        if (matches[i] < current) return matches[i];
    }
    return matches[count - 1];
}

static void apply_history_search(AppState *s, const char *query) {
    int mcount = 0;
    int *matches = collect_history_matches(s, query, &mcount);
    if (!matches) {
        s->search_not_found = 1;
        s->search_match_index = -1;
        return;
    }

    s->history_selected = matches[0];
    s->search_match_index = matches[0];
    s->search_not_found = 0;
    free(matches);
}

static void apply_response_search(AppState *s, const char *query) {
    int mcount = 0;
    int *matches = collect_response_matches(s->response.body_view, query, &mcount);
    if (!matches) {
        s->search_not_found = 1;
        s->search_match_index = -1;
        return;
    }

    s->response_scroll = matches[0];
    s->search_match_index = matches[0];
    s->search_not_found = 0;
    free(matches);
}

void dispatch_apply_search(AppState *s, const char *query) {
    if (!s) return;

    if (!query) query = "";
    strncpy(s->search_query, query, sizeof(s->search_query) - 1);
    s->search_query[sizeof(s->search_query) - 1] = '\0';
    s->search_match_index = -1;
    s->search_not_found = 0;

    if (s->search_query[0] == '\0') return;

    if (s->search_target == SEARCH_TARGET_HISTORY) {
        apply_history_search(s, s->search_query);
        return;
    }
    apply_response_search(s, s->search_query);
}

static void step_history_search(AppState *s, int dir) {
    int mcount = 0;
    int *matches = collect_history_matches(s, s->search_query, &mcount);
    if (!matches) {
        s->search_not_found = 1;
        s->search_match_index = -1;
        return;
    }

    int next = pick_match_with_wrap(matches, mcount, s->search_match_index, dir);
    if (next >= 0) {
        s->history_selected = next;
        s->search_match_index = next;
        s->search_not_found = 0;
    } else {
        s->search_not_found = 1;
        s->search_match_index = -1;
    }
    free(matches);
}

static void step_response_search(AppState *s, int dir) {
    int mcount = 0;
    int *matches = collect_response_matches(s->response.body_view, s->search_query, &mcount);
    if (!matches) {
        s->search_not_found = 1;
        s->search_match_index = -1;
        return;
    }

    int next = pick_match_with_wrap(matches, mcount, s->search_match_index, dir);
    if (next >= 0) {
        s->response_scroll = next;
        s->search_match_index = next;
        s->search_not_found = 0;
    } else {
        s->search_not_found = 1;
        s->search_match_index = -1;
    }
    free(matches);
}

static void dispatch_step_search(AppState *s, int dir) {
    if (!s || s->search_query[0] == '\0') return;
    if (s->search_target == SEARCH_TARGET_HISTORY) {
        step_history_search(s, dir);
    } else {
        step_response_search(s, dir);
    }
}

static void response_reset_content(AppState *s) {
    free(s->response.body);
    free(s->response.body_view);
    free(s->response.error);

    s->response.body = NULL;
    s->response.body_view = NULL;
    s->response.error = NULL;
    s->response.status = 0;
    s->response.elapsed_ms = 0.0;
    s->response.is_json = 0;
    s->response_scroll = 0;
}

static void response_set_text(AppState *s, const char *text) {
    response_reset_content(s);
    s->response.body = strdup(text ? text : "");
    s->response.body_view = strdup(text ? text : "");
    s->focused_panel = PANEL_RESPONSE;
}

static void response_set_error(AppState *s, const char *err) {
    response_reset_content(s);
    s->response.error = strdup(err ? err : "Unknown error");
    s->focused_panel = PANEL_RESPONSE;
}

static int appendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (need < 0) return 0;

    size_t req = *len + (size_t)need + 1;
    if (*cap < req) {
        size_t new_cap = *cap ? *cap : 256;
        while (new_cap < req) new_cap *= 2;
        char *n = realloc(*buf, new_cap);
        if (!n) return 0;
        *buf = n;
        *cap = new_cap;
    }

    va_start(ap, fmt);
    vsnprintf(*buf + *len, *cap - *len, fmt, ap);
    va_end(ap);
    *len += (size_t)need;
    return 1;
}

static const char *mode_name(Mode m) {
    switch (m) {
        case MODE_NORMAL: return "normal";
        case MODE_INSERT: return "insert";
        case MODE_COMMAND: return "command";
        case MODE_SEARCH: return "search";
        default: return "unknown";
    }
}

static int key_name_from_code(int code, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    if (code == 27) return snprintf(out, out_sz, "esc") > 0;
    if (code == 9) return snprintf(out, out_sz, "tab") > 0;
    if (code == KEY_ENTER) return snprintf(out, out_sz, "enter") > 0;
    if (code == KEY_SENTER) return snprintf(out, out_sz, "s-enter") > 0;
    if (code == ' ') return snprintf(out, out_sz, "space") > 0;
    if (code >= 32 && code <= 126) return snprintf(out, out_sz, "%c", (char)code) > 0;
    return 0;
}

static void command_apply_theme(AppState *s, const char *preset, int save) {
    if (!preset || !preset[0]) {
        response_set_error(s, "Usage: :theme <name> [-s|--save]");
        return;
    }

    LayoutTheme themed;
    if (layout_theme_catalog_apply(&s->theme_catalog, preset, &themed) != 0) {
        response_set_error(s, "Unknown theme preset. Use :theme list");
        return;
    }

    s->ui_theme = themed;
    strncpy(s->active_theme_preset, preset, sizeof(s->active_theme_preset) - 1);
    s->active_theme_preset[sizeof(s->active_theme_preset) - 1] = '\0';
    ui_draw_init_theme(s);

    if (save) {
        int rc = layout_save_config(
            "config/layout.conf",
            s->ui_layout_profile,
            s->quad_history_slot,
            s->quad_editor_slot,
            s->quad_response_slot,
            &s->ui_layout_sizing,
            &s->ui_theme,
            s->active_theme_preset
        );

        char msg[256];
        if (rc == 0) {
            snprintf(msg, sizeof(msg), "Theme '%s' applied and saved to config/layout.conf", s->active_theme_preset);
        } else {
            snprintf(
                msg,
                sizeof(msg),
                "Theme '%s' applied for this session, but failed to save config/layout.conf",
                s->active_theme_preset
            );
        }
        response_set_text(s, msg);
        return;
    }

    char msg[192];
    snprintf(msg, sizeof(msg), "Theme '%s' applied for this session", s->active_theme_preset);
    response_set_text(s, msg);
}

static void command_list_themes(AppState *s) {
    char *list = layout_theme_catalog_list_names(&s->theme_catalog);
    if (!list) {
        response_set_error(s, "Out of memory listing themes");
        return;
    }
    response_set_text(s, list);
    free(list);
}

static void command_clear_history(AppState *s) {
    if (!s->history) {
        response_set_error(s, "History is not initialized");
        return;
    }

    if (s->is_request_in_flight) {
        response_set_error(s, "Cannot clear history while request is in flight");
        return;
    }

    history_free(s->history);
    history_init(s->history);
    s->history_selected = 0;
    s->search_match_index = -1;
    s->search_not_found = 0;

    int rc = 1;
    if (s->history_path) {
        rc = history_storage_save(s->history, s->history_path);
    } else {
        char *path = history_storage_default_path();
        if (path) {
            rc = history_storage_save(s->history, path);
            free(path);
        }
    }

    if (rc == 0) {
        response_set_text(s, "History cleared");
    } else {
        response_set_text(s, "History cleared in memory, but failed to persist storage");
    }
}

static char *build_help_text(const Keymap *km) {
    if (!km) return strdup("No keymap loaded.");

    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    if (!appendf(&buf, &len, &cap, "Commands:\n")) goto fail;
    if (!appendf(&buf, &len, &cap, "  :q | :quit  Quit application\n")) goto fail;
    if (!appendf(&buf, &len, &cap, "  :h | :help  Show this help\n")) goto fail;
    if (!appendf(&buf, &len, &cap, "  :theme list  List available theme presets\n")) goto fail;
    if (!appendf(&buf, &len, &cap, "  :theme <name>  Apply theme preset for current session\n")) goto fail;
    if (!appendf(&buf, &len, &cap, "  :theme <name> -s|--save  Apply and persist active preset\n")) goto fail;
    if (!appendf(&buf, &len, &cap, "  :clear! | :ch!  Clear history (memory + storage)\n\n")) goto fail;
    if (!appendf(&buf, &len, &cap, "Key bindings:\n")) goto fail;

    for (int m = 0; m < MODE_COUNT; m++) {
        if (!appendf(&buf, &len, &cap, "[%s]\n", mode_name((Mode)m))) goto fail;
        for (int k = 0; k < KEYMAP_MAX_KEYCODE; k++) {
            Action a = km->table[m][k];
            if (a == ACT_NONE) continue;

            char key_name[32];
            if (!key_name_from_code(k, key_name, sizeof(key_name))) continue;
            const char *desc = action_description(a);
            if (desc && desc[0]) {
                if (!appendf(&buf, &len, &cap, "  %-8s -> %-20s %s\n", key_name, action_to_string(a), desc)) goto fail;
            } else {
                if (!appendf(&buf, &len, &cap, "  %-8s -> %s\n", key_name, action_to_string(a))) goto fail;
            }
        }
        if (!appendf(&buf, &len, &cap, "\n")) goto fail;
    }

    return buf;

fail:
    free(buf);
    return NULL;
}

void dispatch_execute_command(AppState *s, const Keymap *km, const char *cmd) {
    if (!s) return;

    char local[PROMPT_MAX];
    if (!cmd) cmd = "";
    strncpy(local, cmd, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';

    char *p = local;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == ':') p++;
    while (*p && isspace((unsigned char)*p)) p++;

    size_t n = strlen(p);
    while (n > 0 && isspace((unsigned char)p[n - 1])) {
        p[n - 1] = '\0';
        n--;
    }

    if (*p == '\0') {
        s->mode = MODE_NORMAL;
        clear_prompt(s);
        return;
    }

    if (strcmp(p, "q") == 0 || strcmp(p, "quit") == 0) {
        s->running = 0;
        s->mode = MODE_NORMAL;
        clear_prompt(s);
        return;
    }

    if (strcmp(p, "h") == 0 || strcmp(p, "help") == 0) {
        char *help = build_help_text(km);
        if (!help) response_set_error(s, "Out of memory building help text");
        else {
            response_set_text(s, help);
            free(help);
        }
        s->mode = MODE_NORMAL;
        clear_prompt(s);
        return;
    }

    if (strncmp(p, "theme", 5) == 0 && (p[5] == '\0' || isspace((unsigned char)p[5]))) {
        char *args = p + 5;
        while (*args && isspace((unsigned char)*args)) args++;

        if (*args == '\0') {
            response_set_error(s, "Usage: :theme <name> [-s|--save] | :theme list");
            s->mode = MODE_NORMAL;
            clear_prompt(s);
            return;
        }

        char *name = strtok(args, " \t");
        char *flag = strtok(NULL, " \t");
        char *extra = strtok(NULL, " \t");

        if (!name || extra) {
            response_set_error(s, "Usage: :theme <name> [-s|--save] | :theme list");
            s->mode = MODE_NORMAL;
            clear_prompt(s);
            return;
        }

        if (strcmp(name, "list") == 0) {
            if (flag) {
                response_set_error(s, "Usage: :theme list");
                s->mode = MODE_NORMAL;
                clear_prompt(s);
                return;
            }
            command_list_themes(s);
            s->mode = MODE_NORMAL;
            clear_prompt(s);
            return;
        }

        int save = 0;
        if (flag) {
            if (strcmp(flag, "-s") == 0 || strcmp(flag, "--save") == 0) {
                save = 1;
            } else {
                response_set_error(s, "Invalid flag. Use -s or --save");
                s->mode = MODE_NORMAL;
                clear_prompt(s);
                return;
            }
        }

        command_apply_theme(s, name, save);
        s->mode = MODE_NORMAL;
        clear_prompt(s);
        return;
    }

    if (strcmp(p, "clear!") == 0 || strcmp(p, "ch!") == 0) {
        command_clear_history(s);
        s->mode = MODE_NORMAL;
        clear_prompt(s);
        return;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Unknown command: %s", p);
    response_set_error(s, msg);
    s->mode = MODE_NORMAL;
    clear_prompt(s);
}

void dispatch_action(AppState *s, Action a) {
    switch (a) {
        case ACT_QUIT: s->running = 0; break;

        case ACT_ENTER_INSERT:
            s->mode = MODE_INSERT;
            clear_prompt(s);
            break;
        case ACT_ENTER_NORMAL:
            s->mode = MODE_NORMAL;
            clear_prompt(s);
            break;
        case ACT_ENTER_COMMAND:
            s->mode = MODE_COMMAND;
            begin_prompt(s, PROMPT_COMMAND);
            break;
        case ACT_ENTER_SEARCH:
            s->mode = MODE_SEARCH;
            begin_prompt(s, PROMPT_SEARCH);
            s->search_not_found = 0;
            s->search_target = (s->focused_panel == PANEL_HISTORY)
                ? SEARCH_TARGET_HISTORY
                : SEARCH_TARGET_RESPONSE;
            break;

        case ACT_FOCUS_LEFT:
            if (s->focused_panel > 0) s->focused_panel--;
            break;

        case ACT_FOCUS_RIGHT:
            if (s->focused_panel < PANEL_COUNT - 1) s->focused_panel++;
            break;

        case ACT_MOVE_DOWN:
            if (s->focused_panel == PANEL_HISTORY) {
                if (s->history && s->history_selected < s->history->count - 1) {
                    s->history_selected++;
                }
            }else if (s->focused_panel == PANEL_RESPONSE) {
               s->response_scroll++;
            }
            break;

        case ACT_MOVE_UP:
            if (s->focused_panel == PANEL_HISTORY && s->history_selected > 0) {
                s->history_selected--;
            }else if (s->focused_panel == PANEL_RESPONSE) {
               if (s->response_scroll > 0) s->response_scroll--;
            }
            break;

        case ACT_TOGGLE_EDITOR_FIELD:
            if (s->focused_panel == PANEL_EDITOR) {
                if (s->active_field == EDIT_FIELD_URL) s->active_field = EDIT_FIELD_BODY;
                else if (s->active_field == EDIT_FIELD_BODY) s->active_field = EDIT_FIELD_HEADERS;
                else s->active_field = EDIT_FIELD_URL;
            }
            break;

        case ACT_SEND_REQUEST:
            start_request_if_possible(s);
            break;

        case ACT_CYCLE_METHOD:
            s->method = (HttpMethod)((s->method + 1) % HTTP_METHOD_COUNT);
            break;

        case ACT_CYCLE_ENVIRONMENT:
            if (s->mode == MODE_NORMAL) {
                env_store_cycle(&s->envs);
            }
            break;

        case ACT_SEARCH_NEXT:
            if (s->mode == MODE_NORMAL) {
                dispatch_step_search(s, +1);
            }
            break;

        case ACT_SEARCH_PREV:
            if (s->mode == MODE_NORMAL) {
                dispatch_step_search(s, -1);
            }
            break;

        case ACT_HISTORY_LOAD: {
            if (s->focused_panel != PANEL_HISTORY) break;
            if (!s->history) break;

            HistoryItem *it = history_get(s->history, s->history_selected);
            if (!it) break;
            if (!load_history_item_into_state(s, it)) break;
            s->focused_panel = PANEL_EDITOR;
            break;
        }

        case ACT_HISTORY_REPLAY: {
            if (s->focused_panel != PANEL_HISTORY) break;
            if (!s->history) break;
            if (s->is_request_in_flight) break;

            HistoryItem *it = history_get(s->history, s->history_selected);
            if (!it) break;
            if (!load_history_item_into_state(s, it)) break;

            start_request_if_possible(s);
            break;
        }

        default:
            break;
    }
}
