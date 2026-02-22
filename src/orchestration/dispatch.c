/* dispatch.c - Central action dispatcher */

#include "state.h"
#include "core/interaction/actions.h"
#include "core/storage/history.h"
#include "core/storage/history_persistence.h"
#include <pthread.h>
#include "core/http/request_thread.h"
#include "orchestration/dispatch.h"
#include "core/config/env.h"
#include "core/config/layout.h"
#include "core/http/request_snapshot.h"
#include "core/format/export.h"
#include "core/interaction/auth.h"
#include "core/utils/utils.h"
#include "core/interaction/search.h"
#include "core/cli/command_handlers.h"
#include "core/cli/help_builder.h"
#include "core/cli/command_parser.h"
#include "ui/panels/draw.h"
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
    if (s->response.is_request_in_flight) return;

    s->response.is_request_in_flight = 1;
    pthread_t t;
    pthread_create(&t, NULL, request_thread, s);
    pthread_detach(t);  /* Safe: is_request_in_flight prevents races; state_free() waits */
}

static int load_history_item_into_state(AppState *s, const HistoryItem *it) {
    if (!s || !it) return 0;

    s->editor.method = it->method;

    strncpy(s->editor.url, it->url ? it->url : "", sizeof(s->editor.url) - 1);
    s->editor.url[sizeof(s->editor.url) - 1] = '\0';
    s->editor.url_len = strlen(s->editor.url);
    s->editor.url_cursor = s->editor.url_len;

    tb_set_from_string(&s->editor.body, it->body);
    tb_set_from_string(&s->editor.headers, it->headers);

    free(s->response.response.body);
    free(s->response.response.body_view);
    free(s->response.response.error);

    s->response.response.status = it->status;
    s->response.response.elapsed_ms = it->elapsed_ms;
    s->response.response.is_json = it->is_json;
    s->response.response.body = it->response_body ? strdup(it->response_body) : NULL;
    s->response.response.body_view = it->response_body_view ? strdup(it->response_body_view) : NULL;
    s->response.response.error = NULL;
    s->response.scroll = 0;
    s->editor.body_scroll = 0;
    s->editor.headers_scroll = 0;

    return 1;
}

static void clear_prompt(AppState *s) {
    s->prompt.kind = PROMPT_NONE;
    s->prompt.input[0] = '\0';
    s->prompt.len = 0;
    s->prompt.cursor = 0;
}

static void begin_prompt(AppState *s, PromptKind kind) {
    s->prompt.kind = kind;
    s->prompt.input[0] = '\0';
    s->prompt.len = 0;
    s->prompt.cursor = 0;
}

void dispatch_execute_command(AppState *s, const Keymap *km, const char *cmd) {
    command_parse_and_execute(s, km, cmd);
}

void dispatch_action(AppState *s, Action a) {
    switch (a) {
        case ACT_QUIT: s->running = 0; break;

        case ACT_ENTER_INSERT:
            s->ui.mode = MODE_INSERT;
            clear_prompt(s);
            break;
        case ACT_ENTER_NORMAL:
            s->ui.mode = MODE_NORMAL;
            clear_prompt(s);
            break;
        case ACT_ENTER_COMMAND:
            s->ui.mode = MODE_COMMAND;
            begin_prompt(s, PROMPT_COMMAND);
            s->prompt.command_history_index = -1;
            break;
        case ACT_ENTER_SEARCH:
            s->ui.mode = MODE_SEARCH;
            begin_prompt(s, PROMPT_SEARCH);
            s->search.not_found = 0;
            s->search.target = search_get_effective_target(s);
            break;

        case ACT_FOCUS_LEFT:
            if (s->ui.focused_panel > 0) s->ui.focused_panel--;
            break;

        case ACT_FOCUS_RIGHT:
            if (s->ui.focused_panel < PANEL_COUNT - 1) s->ui.focused_panel++;
            break;

        case ACT_MOVE_DOWN:
            if (s->ui.focused_panel == PANEL_HISTORY) {
                if (s->history.history && s->history.selected < s->history.history->count - 1) {
                    s->history.selected++;
                }
            }else if (s->ui.focused_panel == PANEL_RESPONSE) {
               s->response.scroll++;
            }
            break;

        case ACT_MOVE_UP:
            if (s->ui.focused_panel == PANEL_HISTORY && s->history.selected > 0) {
                s->history.selected--;
            }else if (s->ui.focused_panel == PANEL_RESPONSE) {
               if (s->response.scroll > 0) s->response.scroll--;
            }
            break;

        case ACT_TOGGLE_EDITOR_FIELD:
            if (s->ui.focused_panel == PANEL_EDITOR) {
                if (s->editor.active_field == EDIT_FIELD_URL) s->editor.active_field = EDIT_FIELD_BODY;
                else if (s->editor.active_field == EDIT_FIELD_BODY) s->editor.active_field = EDIT_FIELD_HEADERS;
                else s->editor.active_field = EDIT_FIELD_URL;
            }
            break;

        case ACT_SEND_REQUEST:
            start_request_if_possible(s);
            break;

        case ACT_CYCLE_METHOD:
            s->editor.method = (HttpMethod)((s->editor.method + 1) % HTTP_METHOD_COUNT);
            break;

        case ACT_CYCLE_ENVIRONMENT:
            if (s->ui.mode == MODE_NORMAL) {
                env_store_cycle(&s->config.envs);
            }
            break;

        case ACT_SEARCH_NEXT:
            if (s->ui.mode == MODE_NORMAL) {
                search_step(s, +1);
            }
            break;

        case ACT_SEARCH_PREV:
            if (s->ui.mode == MODE_NORMAL) {
                search_step(s, -1);
            }
            break;

        case ACT_HISTORY_LOAD: {
            if (s->ui.focused_panel == PANEL_EDITOR) {
                s->ui.mode = MODE_INSERT;
                clear_prompt(s);
                break;
            }

            if (s->ui.focused_panel != PANEL_HISTORY) break;
            if (!s->history.history) break;

            HistoryItem *it = history_get(s->history.history, s->history.selected);
            if (!it) break;
            if (!load_history_item_into_state(s, it)) break;
            s->ui.focused_panel = PANEL_EDITOR;
            break;
        }

        case ACT_HISTORY_REPLAY: {
            if (s->ui.focused_panel != PANEL_HISTORY) break;
            if (!s->history.history) break;
            if (s->response.is_request_in_flight) break;

            HistoryItem *it = history_get(s->history.history, s->history.selected);
            if (!it) break;
            if (!load_history_item_into_state(s, it)) break;

            start_request_if_possible(s);
            break;
        }

        default:
            break;
    }
}
