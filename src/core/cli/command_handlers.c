#include "core/cli/command_handlers.h"
#include "core/storage/history.h"
#include "core/storage/history_persistence.h"
#include "core/config/env.h"
#include "core/config/layout.h"
#include "core/http/request_snapshot.h"
#include "core/format/export.h"
#include "core/interaction/auth.h"
#include "core/utils/utils.h"
#include "core/interaction/search.h"
#include "core/text/i18n.h"
#include "ui/panels/draw.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Forward declarations of internal helpers */
static void response_reset_content(AppState *s);
static void response_set_text(AppState *s, const char *text);
static void response_set_error(AppState *s, const char *err);

/* Response management helpers */
static void response_reset_content(AppState *s) {
    free(s->response.response.body);
    free(s->response.response.body_view);
    free(s->response.response.response_headers);
    free(s->response.response.error);

    s->response.response.body = NULL;
    s->response.response.body_view = NULL;
    s->response.response.response_headers = NULL;
    s->response.response.error = NULL;
    s->response.response.status = 0;
    s->response.response.elapsed_ms = 0.0;
    s->response.response.is_json = 0;
    s->response.scroll = 0;
}

static void response_set_text(AppState *s, const char *text) {
    response_reset_content(s);
    s->response.response.body = strdup(text ? text : "");
    s->response.response.body_view = strdup(text ? text : "");
    s->ui.focused_panel = PANEL_RESPONSE;
}

static void response_set_error(AppState *s, const char *err) {
    response_reset_content(s);
    s->response.response.error = strdup(err ? err : i18n_get(s->ui.language, I18N_UNKNOWN_ERROR));
    s->ui.focused_panel = PANEL_RESPONSE;
}

/* Command handlers implementation */

void cmd_apply_theme(AppState *s, const char *preset, int save) {
    if (!preset || !preset[0]) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_THEME_NAME_SAVE));
        return;
    }

    LayoutTheme themed;
    if (layout_theme_catalog_apply(&s->ui.theme_catalog, preset, &themed) != 0) {
        response_set_error(s, i18n_get(s->ui.language, I18N_UNKNOWN_THEME_PRESET));
        return;
    }

    s->ui.theme = themed;
    strncpy(s->ui.active_theme_preset, preset, sizeof(s->ui.active_theme_preset) - 1);
    s->ui.active_theme_preset[sizeof(s->ui.active_theme_preset) - 1] = '\0';
    ui_draw_init_theme(s);

    if (save) {
        const char *layout_path = s->config.paths.layout_conf ? s->config.paths.layout_conf : "config/layout.conf";
        int rc = layout_save_config(
            layout_path,
            s->ui.layout_profile,
            s->ui.quad_history_slot,
            s->ui.quad_editor_slot,
            s->ui.quad_response_slot,
            &s->ui.layout_sizing,
            &s->ui.theme,
            s->ui.show_footer_hint,
            s->ui.language_setting,
            s->ui.active_theme_preset
        );

        char msg[256];
        if (rc == 0) {
            snprintf(
                msg,
                sizeof(msg),
                i18n_get(s->ui.language, I18N_THEME_APPLIED_SAVED_FMT),
                s->ui.active_theme_preset,
                layout_path
            );
        } else {
            snprintf(
                msg,
                sizeof(msg),
                i18n_get(s->ui.language, I18N_THEME_APPLIED_SAVE_FAILED_FMT),
                s->ui.active_theme_preset,
                layout_path
            );
        }
        response_set_text(s, msg);
        return;
    }

    char msg[192];
    snprintf(msg, sizeof(msg), i18n_get(s->ui.language, I18N_THEME_APPLIED_SESSION_FMT), s->ui.active_theme_preset);
    response_set_text(s, msg);
}

void cmd_list_themes(AppState *s) {
    char *list = layout_theme_catalog_list_names(&s->ui.theme_catalog);
    if (!list) {
        response_set_error(s, i18n_get(s->ui.language, I18N_OOM_LISTING_THEMES));
        return;
    }
    response_set_text(s, list);
    free(list);
}

void cmd_clear_history(AppState *s) {
    if (!s->history.history) {
        response_set_error(s, i18n_get(s->ui.language, I18N_HISTORY_NOT_INITIALIZED));
        return;
    }

    if (s->response.is_request_in_flight) {
        response_set_error(s, i18n_get(s->ui.language, I18N_CANNOT_CLEAR_HISTORY_IN_FLIGHT));
        return;
    }

    history_free(s->history.history);
    history_init(s->history.history);
    s->history.selected = 0;
    s->search.match_index = -1;
    s->search.not_found = 0;

    int rc = 1;
    if (s->history.path) {
        rc = history_storage_save(s->history.history, s->history.path);
    } else {
        char *path = history_storage_default_path();
        if (path) {
            rc = history_storage_save(s->history.history, path);
            free(path);
        }
    }

    if (rc == 0) {
        response_set_text(s, i18n_get(s->ui.language, I18N_HISTORY_CLEARED));
    } else {
        response_set_text(s, i18n_get(s->ui.language, I18N_HISTORY_CLEARED_SAVE_FAILED));
    }
}

void cmd_export_request(AppState *s, const char *format) {
    if (!format || !format[0]) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_EXPORT));
        return;
    }

    RequestSnapshot snap;
    if (request_snapshot_build_locked(s, &snap) != 0) {
        response_set_error(s, i18n_get(s->ui.language, I18N_OOM_EXPORT_SNAPSHOT));
        return;
    }

    char *out = NULL;
    if (strcmp(format, "curl") == 0) {
        out = export_as_curl(&snap);
    } else if (strcmp(format, "json") == 0) {
        out = export_as_json(&snap);
    } else {
        request_snapshot_free(&snap);
        response_set_error(s, i18n_get(s->ui.language, I18N_UNKNOWN_EXPORT_FORMAT));
        return;
    }

    request_snapshot_free(&snap);
    if (!out) {
        response_set_error(s, i18n_get(s->ui.language, I18N_EXPORT_FAILED));
        return;
    }

    response_set_text(s, out);
    free(out);
}

void cmd_auth(AppState *s, const char *kind, const char *arg) {
    if (!kind || !*kind || !arg || !*arg) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_AUTH));
        return;
    }

    int rc = 1;
    if (strcmp(kind, "bearer") == 0) {
        rc = auth_apply_bearer(&s->editor.headers, arg);
    } else if (strcmp(kind, "basic") == 0) {
        const char *sep = strchr(arg, ':');
        if (!sep) {
            response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_AUTH_BASIC));
            return;
        }

        size_t ulen = (size_t)(sep - arg);
        char *user = malloc(ulen + 1);
        if (!user) {
            response_set_error(s, i18n_get(s->ui.language, I18N_OOM_APPLY_AUTH));
            return;
        }
        memcpy(user, arg, ulen);
        user[ulen] = '\0';
        const char *pass = sep + 1;
        rc = auth_apply_basic(&s->editor.headers, user, pass);
        free(user);
    } else {
        response_set_error(s, i18n_get(s->ui.language, I18N_UNKNOWN_AUTH_KIND));
        return;
    }

    if (rc == 0) response_set_text(s, i18n_get(s->ui.language, I18N_AUTH_UPDATED));
    else response_set_error(s, i18n_get(s->ui.language, I18N_AUTH_UPDATE_FAILED));
}

void cmd_find(AppState *s, const char *query) {
    if (!query || !*query) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_FIND));
        return;
    }
    s->search.target = search_get_effective_target(s);
    search_apply(s, query);
}

void cmd_set(AppState *s, const char *key, const char *value) {
    if (!key || !*key) {
        char msg[256];
        const char *mode = "auto";
        if (s->search.target_override == SEARCH_TARGET_HISTORY) mode = "history";
        if (s->search.target_override == SEARCH_TARGET_RESPONSE) mode = "response";
        snprintf(
            msg,
            sizeof(msg),
            i18n_get(s->ui.language, I18N_SETTINGS_FMT),
            mode,
            s->history.max_entries
        );
        response_set_text(s, msg);
        return;
    }

    if (strcmp(key, "search_target") == 0) {
        if (!value) {
            response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_SET_SEARCH_TARGET));
            return;
        }
        if (strcmp(value, "auto") == 0) s->search.target_override = -1;
        else if (strcmp(value, "history") == 0) s->search.target_override = SEARCH_TARGET_HISTORY;
        else if (strcmp(value, "response") == 0) s->search.target_override = SEARCH_TARGET_RESPONSE;
        else {
            response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_SET_SEARCH_TARGET));
            return;
        }
        response_set_text(s, i18n_get(s->ui.language, I18N_SEARCH_TARGET_UPDATED));
        return;
    }

    if (strcmp(key, "max_entries") == 0) {
        if (!value) {
            response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_SET_MAX_ENTRIES));
            return;
        }
        char *end = NULL;
        long n = strtol(value, &end, 10);
        if (!end || *end != '\0' || n <= 0 || n > 1000000) {
            response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_SET_MAX_ENTRIES));
            return;
        }
        s->history.max_entries = (int)n;
        if (s->history.history) history_trim_oldest(s->history.history, s->history.max_entries);
        response_set_text(s, i18n_get(s->ui.language, I18N_MAX_ENTRIES_UPDATED_SESSION));
        return;
    }

    response_set_error(s, i18n_get(s->ui.language, I18N_UNKNOWN_SETTING));
}

void cmd_lang(AppState *s, const char *arg) {
    if (!arg || !arg[0]) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_LANG));
        return;
    }

    if (strcmp(arg, "list") == 0) {
        response_set_text(s, "Available languages:\n- auto (detect from LANG env)\n- en (English)\n- pt (Portuguese)");
        return;
    }

    UiLanguageSetting setting;
    if (i18n_parse_language_setting(arg, &setting) != 0) {
        response_set_error(s, i18n_get(s->ui.language, I18N_UNKNOWN_LANGUAGE));
        return;
    }

    s->ui.language_setting = setting;
    s->ui.language = i18n_resolve_language(setting, getenv("LANG"));

    char msg[128];
    snprintf(msg, sizeof(msg), i18n_get(s->ui.language, I18N_LANGUAGE_UPDATED_FMT), 
             i18n_language_setting_name(setting));
    response_set_text(s, msg);
}

void cmd_layout(AppState *s, const char *arg) {
    if (!arg || !arg[0]) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_LAYOUT));
        return;
    }

    if (strcmp(arg, "list") == 0) {
        response_set_text(s, "Available layouts:\n- classic (History left, Editor/Response right)\n- quad (2x2 quadrant)\n- focus_editor (Large editor on top)");
        return;
    }

    LayoutProfile profile;
    if (strcmp(arg, "classic") == 0) {
        profile = LAYOUT_PROFILE_CLASSIC;
    } else if (strcmp(arg, "quad") == 0) {
        profile = LAYOUT_PROFILE_QUAD;
    } else if (strcmp(arg, "focus_editor") == 0) {
        profile = LAYOUT_PROFILE_FOCUS_EDITOR;
    } else {
        response_set_error(s, i18n_get(s->ui.language, I18N_UNKNOWN_LAYOUT));
        return;
    }

    s->ui.layout_profile = profile;

    char msg[128];
    snprintf(msg, sizeof(msg), i18n_get(s->ui.language, I18N_LAYOUT_UPDATED_FMT), 
             layout_profile_name(profile));
    response_set_text(s, msg);
}

void cmd_cookies_list(AppState *s) {
    if (!s->config.paths.cookie_jar) {
        response_set_error(s, "Cookie jar path not configured");
        return;
    }

    FILE *f = fopen(s->config.paths.cookie_jar, "r");
    if (!f) {
        response_set_text(s, "No cookies stored yet");
        return;
    }

    /* Read and display cookie file contents */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size <= 0) {
        fclose(f);
        response_set_text(s, "Cookie jar is empty");
        return;
    }

    fseek(f, 0, SEEK_SET);
    char *content = malloc(size + 1);
    if (!content) {
        fclose(f);
        response_set_error(s, i18n_get(s->ui.language, I18N_UNKNOWN_ERROR));
        return;
    }

    size_t read_size = fread(content, 1, size, f);
    content[read_size] = '\0';
    fclose(f);

    response_set_text(s, content);
    free(content);
}

void cmd_cookies_clear(AppState *s) {
    if (!s->config.paths.cookie_jar) {
        response_set_error(s, "Cookie jar path not configured");
        return;
    }

    /* Delete cookie file */
    if (remove(s->config.paths.cookie_jar) != 0) {
        response_set_text(s, "No cookies to clear");
        return;
    }

    response_set_text(s, i18n_get(s->ui.language, I18N_COOKIES_CLEARED));
}
