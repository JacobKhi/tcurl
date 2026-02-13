#pragma once
#include "state.h"
#include "core/actions.h"
#include "core/keymap.h"

void dispatch_action(AppState *s, Action a);
void dispatch_apply_search(AppState *s, const char *query);
void dispatch_execute_command(AppState *s, const Keymap *km, const char *cmd);
