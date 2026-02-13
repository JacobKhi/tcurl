#include "state.h"
#include "core/actions.h"
#include <pthread.h>
#include "core/request_thread.h"
#include "core/dispatch.h"

void dispatch_action(AppState *s, Action a) {
    switch (a) {
        case ACT_QUIT: s->running = 0; break;

        case ACT_ENTER_INSERT: s->mode = MODE_INSERT; break;
        case ACT_ENTER_NORMAL: s->mode = MODE_NORMAL; break;
        case ACT_ENTER_COMMAND: s->mode = MODE_COMMAND; break;
        case ACT_ENTER_SEARCH: s->mode = MODE_SEARCH; break;

        case ACT_FOCUS_LEFT:
            if (s->focused_panel > 0) s->focused_panel--;
            break;

        case ACT_FOCUS_RIGHT:
            if (s->focused_panel < PANEL_COUNT - 1) s->focused_panel++;
            break;

        case ACT_MOVE_DOWN:
            if (s->focused_panel == PANEL_HISTORY) {
                s->history_selected++;
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
            if (!s->is_request_in_flight) {
                s->is_request_in_flight = 1;
                pthread_t t;
                pthread_create(&t, NULL, request_thread, s);
                pthread_detach(t);
            }
            break;

        case ACT_CYCLE_METHOD:
            s->method = (HttpMethod)((s->method + 1) % HTTP_METHOD_COUNT);
            break;

        default:
            break;
    }
}
