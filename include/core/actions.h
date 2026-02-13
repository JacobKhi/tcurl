#pragma once

typedef enum {
    ACT_NONE = 0,

    ACT_QUIT,
    ACT_MOVE_DOWN,
    ACT_MOVE_UP,
    ACT_FOCUS_LEFT,
    ACT_FOCUS_RIGHT,
    ACT_ENTER_INSERT,
    ACT_ENTER_NORMAL,
    ACT_ENTER_COMMAND,
    ACT_ENTER_SEARCH,
    ACT_SEND_REQUEST,
    ACT_TOGGLE_EDITOR_FIELD,
    ACT_CYCLE_METHOD,

    ACT_COUNT
} Action;

Action action_from_string(const char *name);

const char *action_to_string(Action a);