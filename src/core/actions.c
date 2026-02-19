#include "core/actions.h"
#include "core/i18n.h"
#include <string.h>

typedef struct {
    const char *name;
    Action action;
} ActionEntry;

static const ActionEntry ACTIONS[] = {
    {"quit", ACT_QUIT},
    {"move_down", ACT_MOVE_DOWN},
    {"move_up", ACT_MOVE_UP},
    {"focus_left", ACT_FOCUS_LEFT},
    {"focus_right", ACT_FOCUS_RIGHT},
    {"enter_insert", ACT_ENTER_INSERT},
    {"enter_normal", ACT_ENTER_NORMAL},
    {"enter_command", ACT_ENTER_COMMAND},
    {"enter_search", ACT_ENTER_SEARCH},
    {"send_request", ACT_SEND_REQUEST},
    {"toggle_editor_field", ACT_TOGGLE_EDITOR_FIELD},
    {"cycle_method", ACT_CYCLE_METHOD},
    {"cycle_environment", ACT_CYCLE_ENVIRONMENT},
    {"history_load", ACT_HISTORY_LOAD},
    {"history_replay", ACT_HISTORY_REPLAY},
    {"search_next", ACT_SEARCH_NEXT},
    {"search_prev", ACT_SEARCH_PREV},
};

Action action_from_string(const char *name) {
    if (!name) return ACT_NONE;
    for (unsigned i = 0; i < sizeof(ACTIONS)/sizeof(ACTIONS[0]); i++) {
        if (strcmp(name, ACTIONS[i].name) == 0) return ACTIONS[i].action;
    }
    return ACT_NONE;
}

const char *action_to_string(Action a) {
    for (unsigned i = 0; i < sizeof(ACTIONS)/sizeof(ACTIONS[0]); i++) {
        if (ACTIONS[i].action == a) return ACTIONS[i].name;
    }
    return "none";
}

const char *action_description(Action a, UiLanguage lang) {
    switch (a) {
        case ACT_QUIT: return i18n_get(lang, I18N_ACT_QUIT_DESC);
        case ACT_MOVE_DOWN: return i18n_get(lang, I18N_ACT_MOVE_DOWN_DESC);
        case ACT_MOVE_UP: return i18n_get(lang, I18N_ACT_MOVE_UP_DESC);
        case ACT_FOCUS_LEFT: return i18n_get(lang, I18N_ACT_FOCUS_LEFT_DESC);
        case ACT_FOCUS_RIGHT: return i18n_get(lang, I18N_ACT_FOCUS_RIGHT_DESC);
        case ACT_ENTER_INSERT: return i18n_get(lang, I18N_ACT_ENTER_INSERT_DESC);
        case ACT_ENTER_NORMAL: return i18n_get(lang, I18N_ACT_ENTER_NORMAL_DESC);
        case ACT_ENTER_COMMAND: return i18n_get(lang, I18N_ACT_ENTER_COMMAND_DESC);
        case ACT_ENTER_SEARCH: return i18n_get(lang, I18N_ACT_ENTER_SEARCH_DESC);
        case ACT_SEND_REQUEST: return i18n_get(lang, I18N_ACT_SEND_REQUEST_DESC);
        case ACT_TOGGLE_EDITOR_FIELD: return i18n_get(lang, I18N_ACT_TOGGLE_EDITOR_FIELD_DESC);
        case ACT_CYCLE_METHOD: return i18n_get(lang, I18N_ACT_CYCLE_METHOD_DESC);
        case ACT_CYCLE_ENVIRONMENT: return i18n_get(lang, I18N_ACT_CYCLE_ENV_DESC);
        case ACT_HISTORY_LOAD: return i18n_get(lang, I18N_ACT_HISTORY_LOAD_DESC);
        case ACT_HISTORY_REPLAY: return i18n_get(lang, I18N_ACT_HISTORY_REPLAY_DESC);
        case ACT_SEARCH_NEXT: return i18n_get(lang, I18N_ACT_SEARCH_NEXT_DESC);
        case ACT_SEARCH_PREV: return i18n_get(lang, I18N_ACT_SEARCH_PREV_DESC);
        default: return "";
    }
}
