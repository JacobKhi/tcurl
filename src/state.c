#include "state.h"
#include <string.h>
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
    s->active_field = EDIT_FIELD_URL;
}

void app_state_destroy(AppState *s) {
    tb_free(&s->body);
}
