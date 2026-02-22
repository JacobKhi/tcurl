#include "core/cli/help_builder.h"
#include "core/interaction/actions.h"
#include "core/utils/utils.h"
#include "core/text/i18n.h"
#include "state.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KEY_SENTER
#define KEY_SENTER 548
#endif

/* Helper: Get localized mode name */
static const char *mode_name(Mode m, UiLanguage lang) {
    switch (m) {
        case MODE_NORMAL: return i18n_get(lang, I18N_MODE_NORMAL_LC);
        case MODE_INSERT: return i18n_get(lang, I18N_MODE_INSERT_LC);
        case MODE_COMMAND: return i18n_get(lang, I18N_MODE_COMMAND_LC);
        case MODE_SEARCH: return i18n_get(lang, I18N_MODE_SEARCH_LC);
        default: return i18n_get(lang, I18N_MODE_UNKNOWN_LC);
    }
}

/* Helper: Convert key code to human-readable name */
static int key_name_from_code(int code, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return 0;
    if (code == 27) return snprintf(out, out_sz, "esc") > 0;
    if (code == 9) return snprintf(out, out_sz, "tab") > 0;
    if (code == KEY_ENTER) return snprintf(out, out_sz, "enter") > 0;
    if (code == KEY_SENTER) return snprintf(out, out_sz, "s-enter") > 0;
    if (code == KEY_LEFT) return snprintf(out, out_sz, "left") > 0;
    if (code == KEY_RIGHT) return snprintf(out, out_sz, "right") > 0;
    if (code == KEY_UP) return snprintf(out, out_sz, "up") > 0;
    if (code == KEY_DOWN) return snprintf(out, out_sz, "down") > 0;
    if (code == ' ') return snprintf(out, out_sz, "space") > 0;
    if (code >= 32 && code <= 126) return snprintf(out, out_sz, "%c", (char)code) > 0;
    return 0;
}

/**
 * Append basic commands section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_basic(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_BASIC))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_QUIT))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_HELP))) return 0;
    return 1;
}

/**
 * Append language commands section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_lang(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_LANG))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_LANG_LIST))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_LANG_SET))) return 0;
    return 1;
}

/**
 * Append layout commands section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_layout(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_LAYOUT))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_LAYOUT_LIST))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_LAYOUT_SET))) return 0;
    return 1;
}

/**
 * Append theme management section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_theme(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_THEME))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_THEME_LIST))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_THEME_APPLY))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_THEME_SAVE))) return 0;
    return 1;
}

/**
 * Append export section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_export(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_EXPORT))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_EXPORT))) return 0;
    return 1;
}

/**
 * Append authentication section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_auth(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_AUTH))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_AUTH_BEARER))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_AUTH_BASIC))) return 0;
    return 1;
}

/**
 * Append search section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_search(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_SEARCH))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_FIND))) return 0;
    return 1;
}

/**
 * Append history section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_history(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_HISTORY))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_CLEAR))) return 0;
    return 1;
}

/**
 * Append settings section to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_settings(char **buf, size_t *len, size_t *cap, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_HEADER_SETTINGS))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_CMD_SET))) return 0;
    return 1;
}

/**
 * Append navigation and key bindings sections to help text buffer.
 * Returns 0 on failure, 1 on success.
 */
static int append_help_keys(char **buf, size_t *len, size_t *cap, const Keymap *km, UiLanguage lang) {
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_NAV_HEADER))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_NAV_LINE))) return 0;
    if (!str_appendf(buf, len, cap, "%s", i18n_get(lang, I18N_HELP_KEYS_HEADER))) return 0;

    for (int m = 0; m < MODE_COUNT; m++) {
        if (!str_appendf(buf, len, cap, "[%s]\n", mode_name((Mode)m, lang))) return 0;
        for (int k = 0; k < KEYMAP_MAX_KEYCODE; k++) {
            Action a = km->table[m][k];
            if (a == ACT_NONE) continue;

            char key_name[32];
            if (!key_name_from_code(k, key_name, sizeof(key_name))) continue;
            const char *desc = action_description(a, lang);
            if (!desc || !desc[0]) {
                desc = i18n_get(lang, I18N_HELP_NO_DESC);
            }
            if (!str_appendf(buf, len, cap, "  %-12s -> %s\n", key_name, desc)) return 0;
        }
        if (!str_appendf(buf, len, cap, "\n")) return 0;
    }

    return 1;
}

char *help_build_text(const Keymap *km, UiLanguage lang) {
    if (!km) return strdup(i18n_get(lang, I18N_NO_KEYMAP_LOADED));

    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    if (!append_help_basic(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_lang(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_layout(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_theme(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_export(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_auth(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_search(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_history(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_settings(&buf, &len, &cap, lang)) goto fail;
    if (!append_help_keys(&buf, &len, &cap, km, lang)) goto fail;

    return buf;

fail:
    free(buf);
    return NULL;
}
