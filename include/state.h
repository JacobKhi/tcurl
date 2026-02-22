#pragma once
#include <pthread.h>
#include "core/text/textbuf.h"
#include "core/config/env.h"
#include "core/config/layout.h"
#include "core/storage/paths.h"
#include "core/text/i18n.h"
#include "core/config/constants.h"

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

/* UI State - Mode, layout, theme, language */
typedef struct {
    Mode mode;
    Panel focused_panel;
    LayoutProfile layout_profile;
    LayoutSlot quad_history_slot;
    LayoutSlot quad_editor_slot;
    LayoutSlot quad_response_slot;
    LayoutSizing layout_sizing;
    LayoutTheme theme;
    int show_footer_hint;
    UiLanguageSetting language_setting;
    UiLanguage language;
    ThemeCatalog theme_catalog;
    char active_theme_preset[64];
} UIState;

/* Editor State - URL, body, headers, method */
typedef struct {
    char url[URL_MAX];
    int url_len;
    int url_cursor;
    TextBuffer body;
    TextBuffer headers;
    int body_scroll;
    int headers_scroll;
    EditField active_field;
    HttpMethod method;
} EditorState;

/* Response State - HTTP response, status, scroll */
typedef struct {
    HttpResponse response;
    int is_request_in_flight;
    int scroll;
} ResponseState;

/* History State - History entries, selection, persistence */
typedef struct {
    History *history;
    int selected;
    int max_entries;
    char *path;
    int loaded_ok;
    int skipped_invalid;
    int last_save_error;
} HistoryState;

/* Config State - Environments, paths, suggestions */
typedef struct {
    AppPaths paths;
    EnvStore envs;
    char **header_suggestions;
    int header_suggestions_count;
    /* Header autocomplete state */
    int headers_ac_row;
    int headers_ac_next_match;
    char *headers_ac_seed;
} ConfigState;

/* Prompt State - Command/search prompt */
typedef struct {
    PromptKind kind;
    char input[PROMPT_MAX];
    int len;
    int cursor;
    char command_history[CMD_HISTORY_MAX][PROMPT_MAX];
    int command_history_count;
    int command_history_index;
} PromptState;

/* Search State - Search functionality */
typedef struct {
    char query[PROMPT_MAX];
    SearchTarget target;
    int target_override; /* -1 auto, else SearchTarget */
    int match_index;
    int not_found;
} SearchState;

/* Main Application State - Composed of sub-states */
typedef struct {
    int running;
    pthread_mutex_t state_mu;
    
    /* Sub-states - organized by concern */
    UIState ui;
    EditorState editor;
    ResponseState response;
    HistoryState history;
    ConfigState config;
    PromptState prompt;
    SearchState search;
} AppState;

void app_state_init(AppState *s);

void app_state_destroy(AppState *s);

void app_state_lock(AppState *s);
void app_state_unlock(AppState *s);
