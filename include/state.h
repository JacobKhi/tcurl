#pragma once
#include "core/textbuf.h"

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

typedef enum {
    EDIT_FIELD_URL = 0,
    EDIT_FIELD_BODY = 1
} EditField;

typedef struct {
    long status;
    char *body;
} HttpResponse;

typedef struct {
    int running;
    Mode mode;

    Panel focused_panel;

    int history_selected;

    char url[URL_MAX];
    int url_len;
    int url_cursor;

    TextBuffer body;

    EditField active_field;

    HttpResponse response;
    int is_request_in_flight;
} AppState;

void app_state_init(AppState *s);

void app_state_destroy(AppState *s);