#include "core/actions.h"
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

const char *action_description(Action a) {
    switch (a) {
        case ACT_QUIT: return "Quit application";
        case ACT_MOVE_DOWN: return "Move down in focused panel";
        case ACT_MOVE_UP: return "Move up in focused panel";
        case ACT_FOCUS_LEFT: return "Focus panel to the left";
        case ACT_FOCUS_RIGHT: return "Focus panel to the right";
        case ACT_ENTER_INSERT: return "Enter insert mode";
        case ACT_ENTER_NORMAL: return "Return to normal mode";
        case ACT_ENTER_COMMAND: return "Open command prompt";
        case ACT_ENTER_SEARCH: return "Open search prompt";
        case ACT_SEND_REQUEST: return "Send current request";
        case ACT_TOGGLE_EDITOR_FIELD: return "Cycle editor field";
        case ACT_CYCLE_METHOD: return "Cycle HTTP method";
        case ACT_CYCLE_ENVIRONMENT: return "Cycle active environment";
        case ACT_HISTORY_LOAD: return "Load selected history item";
        case ACT_HISTORY_REPLAY: return "Replay selected history request";
        case ACT_SEARCH_NEXT: return "Go to next search match";
        case ACT_SEARCH_PREV: return "Go to previous search match";
        default: return "";
    }
}
