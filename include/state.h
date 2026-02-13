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
    EDIT_FIELD_BODY = 1,
    EDIT_FIELD_HEADERS = 2
} EditField;

typedef struct {
    long status;
    char *body; 
    char *body_view;
} HttpResponse;

typedef enum {
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_METHOD_COUNT
} HttpMethod;

typedef struct {
    int running;
    Mode mode;

    Panel focused_panel;

    int history_selected;

    char url[URL_MAX];
    int url_len;
    int url_cursor;

    TextBuffer body;
    TextBuffer headers;

    EditField active_field;

    HttpResponse response;
    int is_request_in_flight;
    int response_scroll;

    HttpMethod method;
} AppState;

void app_state_init(AppState *s);

void app_state_destroy(AppState *s);