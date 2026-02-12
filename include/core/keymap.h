#pragma once
#include "state.h"
#include "core/actions.h"

#define KEYMAP_MAX_KEYCODE 2048

typedef struct {
    Action table[MODE_COUNT][KEYMAP_MAX_KEYCODE];
} Keymap;

int keymap_load_file(Keymap *km, const char *path);

Action keymap_resolve(const Keymap *km, Mode mode, int keycode);
