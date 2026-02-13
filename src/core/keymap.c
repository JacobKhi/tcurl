#include "core/keymap.h"
#include <ncurses.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef KEY_SENTER
#define KEY_SENTER 548
#endif

static void keymap_clear(Keymap *km) {
    for (int m = 0; m < MODE_COUNT; m++) {
        for (int k = 0; k < KEYMAP_MAX_KEYCODE; k++) {
            km->table[m][k] = ACT_NONE;
        }
    }
}

static void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);

    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
}

static int mode_from_section(const char *sec) {
    if (strcmp(sec, "normal") == 0) return MODE_NORMAL;
    if (strcmp(sec, "insert") == 0) return MODE_INSERT;
    if (strcmp(sec, "command") == 0) return MODE_COMMAND;
    if (strcmp(sec, "search") == 0) return MODE_SEARCH;
    return -1;
}

static int keycode_from_token(char *tok) {
    trim(tok);

    size_t len = strlen(tok);
    if (len >= 3 && tok[0] == '"' && tok[len - 1] == '"') {
        if (len == 3) return (unsigned char)tok[1];
        return -1;
    }

    if (strcmp(tok, "esc") == 0) return 27;
    if (strcmp(tok, "tab") == 0) return 9;
    if (strcmp(tok, "enter") == 0) return KEY_ENTER;
    if (strcmp(tok, "s-enter") == 0) return KEY_SENTER;
    if (strcmp(tok, "left") == 0) return KEY_LEFT;
    if (strcmp(tok, "right") == 0) return KEY_RIGHT;
    if (strcmp(tok, "up") == 0) return KEY_UP;
    if (strcmp(tok, "down") == 0) return KEY_DOWN;

    if (strlen(tok) == 1) return (unsigned char)tok[0];

    return -1;
}

int keymap_load_file(Keymap *km, const char *path) {
    keymap_clear(km);

    FILE *f = fopen(path, "r");
    if (!f) return 1;

    char line[512];
    char section[64] = {0};
    int current_mode = -1;

    while (fgets(line, sizeof(line), f)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';

        trim(line);
        if (line[0] == '\0') continue;

        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (!end) continue;
            *end = '\0';
            size_t sec_len = strlen(line + 1);
            if (sec_len >= sizeof(section)) sec_len = sizeof(section) - 1;
            memcpy(section, line + 1, sec_len);
            section[sec_len] = '\0';
            trim(section);
            current_mode = mode_from_section(section);
            continue;
        }

        char *eq = strchr(line, '=');
        if (!eq || current_mode < 0) continue;

        *eq = '\0';
        char *key_tok = line;
        char *act_tok = eq + 1;

        trim(key_tok);
        trim(act_tok);

        int keycode = keycode_from_token(key_tok);
        Action action = action_from_string(act_tok);

        if (keycode >= 0 && keycode < KEYMAP_MAX_KEYCODE && action != ACT_NONE) {
            km->table[current_mode][keycode] = action;
        }
    }

    fclose(f);
    return 0;
}

Action keymap_resolve(const Keymap *km, Mode mode, int keycode) {
    if (!km) return ACT_NONE;
    if (mode < 0 || mode >= MODE_COUNT) return ACT_NONE;
    if (keycode < 0 || keycode >= KEYMAP_MAX_KEYCODE) return ACT_NONE;

    Action a = km->table[mode][keycode];
    if (a != ACT_NONE) return a;

    if (keycode == '\n' || keycode == '\r') {
        if (KEY_ENTER >= 0 && KEY_ENTER < KEYMAP_MAX_KEYCODE) {
            a = km->table[mode][KEY_ENTER];
            if (a != ACT_NONE) return a;
        }
    }

    if (keycode == KEY_ENTER) {
        a = km->table[mode]['\n'];
        if (a != ACT_NONE) return a;
        a = km->table[mode]['\r'];
        if (a != ACT_NONE) return a;
    }

    /* Backward-compatible vim-parallel navigation for old keymap files:
       if arrows are not explicitly bound, mirror h/j/k/l bindings in normal mode. */
    if (mode == MODE_NORMAL) {
        if (keycode == KEY_LEFT) {
            a = km->table[mode]['h'];
            if (a != ACT_NONE) return a;
        } else if (keycode == KEY_RIGHT) {
            a = km->table[mode]['l'];
            if (a != ACT_NONE) return a;
        } else if (keycode == KEY_DOWN) {
            a = km->table[mode]['j'];
            if (a != ACT_NONE) return a;
        } else if (keycode == KEY_UP) {
            a = km->table[mode]['k'];
            if (a != ACT_NONE) return a;
        }
    }

    return ACT_NONE;
}
