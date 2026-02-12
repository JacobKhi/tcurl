#pragma once

#define URL_MAX 1024

typedef enum {
    MODE_NORMAL = 0,
    MODE_INSERT = 1,
    MODE_COMMAND = 2,
    MODE_SEARCH = 3,
    MODE_COUNT
} Mode;

typedef enum {
    PANEL_HISTORY = 0,
    PANEL_EDITOR  = 1,
    PANEL_RESPONSE = 2,
    PANEL_COUNT
} Panel;

typedef struct {
    int running;
    Mode mode;

    Panel focused_panel;

    int history_selected;

    char url[URL_MAX];
    int url_len;
    int url_cursor;
} AppState;

void app_state_init(AppState *s);
