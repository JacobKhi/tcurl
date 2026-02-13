#include "state.h"
#include <string.h>
#include <stdlib.h>
#include "core/history.h"
#include "core/textbuf.h"

void app_state_init(AppState *s) {
    s->running = 1;
    s->mode = MODE_NORMAL;
    s->focused_panel = PANEL_HISTORY;
    s->history_selected = 0;

    memset(s->url, 0, sizeof(s->url));
    s->url_len = 0;
    s->url_cursor = 0;

    tb_init(&s->body);
    tb_init(&s->headers);
    s->active_field = EDIT_FIELD_URL;

    s->response.status = 0;
    s->response.body = NULL;
    s->is_request_in_flight = 0;

    s->method = HTTP_GET;

    s->response_scroll = 0;

    s->response.body = NULL;
    s->response.body_view = NULL;
    s->response.elapsed_ms = 0.0;
    s->response.error = NULL;

    s->history = malloc(sizeof(*s->history));
    if (s->history) {
        history_init(s->history);
    }
    s->history_selected = 0;

}

void app_state_destroy(AppState *s) {
    tb_free(&s->body);
    tb_free(&s->headers);

    free(s->response.body);
    free(s->response.body_view);
    free(s->response.error);

    if (s->history) {
        history_free(s->history);
        free(s->history);
        s->history = NULL;
    }
}
