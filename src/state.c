#include "state.h"
#include "string.h"

void app_state_init(AppState *s) {
    s->running = 1;
    s->mode = MODE_NORMAL;
    s->focused_panel = PANEL_HISTORY;
    s->history_selected = 0;

    memset(s->url, 0, sizeof(s->url));
    s->url_len = 0;
    s->url_cursor = 0;
}
