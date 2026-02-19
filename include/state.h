#pragma once
#include <pthread.h>
#include "core/textbuf.h"
#include "core/env.h"
#include "core/layout.h"
#include "core/paths.h"
#include "core/i18n.h"

#define URL_MAX 1024
#define PROMPT_MAX 256
#define CMD_HISTORY_MAX 32

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

typedef enum {
    SEARCH_TARGET_HISTORY = 0,
    SEARCH_TARGET_RESPONSE = 1
} SearchTarget;

typedef enum {
    PROMPT_NONE = 0,
    PROMPT_COMMAND = 1,
    PROMPT_SEARCH = 2
} PromptKind;

typedef struct HttpResponse {
    long status;
    char *body; 
    char *body_view;

    double elapsed_ms;
    char *error;
    int is_json;
} HttpResponse;

typedef enum {
    HTTP_GET = 0,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_METHOD_COUNT
} HttpMethod;

typedef struct History History;

typedef struct {
    int running;
    Mode mode;
    pthread_mutex_t state_mu;

    Panel focused_panel;
    LayoutProfile ui_layout_profile;
    LayoutSlot quad_history_slot;
    LayoutSlot quad_editor_slot;
    LayoutSlot quad_response_slot;
    LayoutSizing ui_layout_sizing;
    LayoutTheme ui_theme;
    int ui_show_footer_hint;
    UiLanguageSetting ui_language_setting;
    UiLanguage ui_language;
    ThemeCatalog theme_catalog;
    char active_theme_preset[64];
    AppPaths paths;

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

    History *history;
    int history_max_entries;
    char *history_path;
    int history_loaded_ok;
    int history_skipped_invalid;
    int history_last_save_error;

    EnvStore envs;
    char **header_suggestions;
    int header_suggestions_count;

    int headers_ac_row;
    int headers_ac_next_match;
    char *headers_ac_seed;

    PromptKind prompt_kind;
    char prompt_input[PROMPT_MAX];
    int prompt_len;
    int prompt_cursor;
    char command_history[CMD_HISTORY_MAX][PROMPT_MAX];
    int command_history_count;
    int command_history_index;

    char search_query[PROMPT_MAX];
    SearchTarget search_target;
    int search_target_override; /* -1 auto, else SearchTarget */
    int search_match_index;
    int search_not_found;

    int body_scroll;
    int headers_scroll;
} AppState;

void app_state_init(AppState *s);

void app_state_destroy(AppState *s);

void app_state_lock(AppState *s);
void app_state_unlock(AppState *s);
