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
