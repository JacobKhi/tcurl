#include "state.h"
#include <string.h>
#include <stdlib.h>
#include "core/history.h"
#include "core/history_storage.h"
#include "core/env.h"
#include "core/layout.h"
#include "core/paths.h"
#include "core/textbuf.h"
#include <unistd.h>

void app_state_lock(AppState *s) {
    if (!s) return;
    (void)pthread_mutex_lock(&s->state_mu);
}

void app_state_unlock(AppState *s) {
    if (!s) return;
    (void)pthread_mutex_unlock(&s->state_mu);
}

void app_state_init(AppState *s) {
    memset(s, 0, sizeof(*s));
    (void)pthread_mutex_init(&s->state_mu, NULL);

    paths_init(&s->paths);
    if (paths_resolve_config_dir(&s->paths) != 0 || paths_build_file_paths(&s->paths) != 0) {
        paths_free(&s->paths);
        paths_init(&s->paths);
    }

    s->running = 1;
    s->mode = MODE_NORMAL;
    s->focused_panel = PANEL_HISTORY;
    s->ui_layout_profile = LAYOUT_PROFILE_CLASSIC;
    s->quad_history_slot = LAYOUT_SLOT_TL;
    s->quad_editor_slot = LAYOUT_SLOT_TR;
    s->quad_response_slot = LAYOUT_SLOT_BR;
    memset(s->active_theme_preset, 0, sizeof(s->active_theme_preset));
    (void)layout_load_config(
        s->paths.layout_conf_load ? s->paths.layout_conf_load : "config/layout.conf",
        &s->ui_layout_profile,
        &s->quad_history_slot,
        &s->quad_editor_slot,
        &s->quad_response_slot,
        &s->ui_layout_sizing,
        &s->ui_theme,
        &s->ui_show_footer_hint,
        &s->ui_language_setting,
        s->active_theme_preset,
        sizeof(s->active_theme_preset)
    );
    s->ui_language = i18n_resolve_language(s->ui_language_setting, getenv("LANG"));
    layout_theme_catalog_init(&s->theme_catalog);
    (void)layout_theme_catalog_load(
        s->paths.themes_conf ? s->paths.themes_conf : "config/themes.conf",
        &s->theme_catalog
    );
    if (s->active_theme_preset[0] != '\0') {
        LayoutTheme themed;
        if (layout_theme_catalog_apply(&s->theme_catalog, s->active_theme_preset, &themed) == 0) {
            s->ui_theme = themed;
        } else {
            s->active_theme_preset[0] = '\0';
        }
    }
    s->history_selected = 0;

    memset(s->url, 0, sizeof(s->url));
    s->url_len = 0;
    s->url_cursor = 0;

    tb_init(&s->body);
    tb_init(&s->headers);
    s->active_field = EDIT_FIELD_URL;

    s->response.status = 0;
    s->response.body = NULL;
    s->is_request_in_flight = 0;

    s->method = HTTP_GET;

    s->response_scroll = 0;

    s->response.body = NULL;
    s->response.body_view = NULL;
    s->response.elapsed_ms = 0.0;
    s->response.error = NULL;

    s->history_max_entries = history_config_load_max_entries(
        s->paths.history_conf ? s->paths.history_conf : "config/history.conf",
        500
    );
    s->history_path = history_storage_default_path();
    if (!s->history_path) {
        s->history_path = strdup("./history.jsonl");
    }

    s->history_loaded_ok = 0;
    s->history_skipped_invalid = 0;
    s->history_last_save_error = 0;
    s->history = malloc(sizeof(*s->history));
    if (s->history) {
        HistoryLoadStats hs = {0};
        history_init(s->history);
        (void)history_storage_load_with_stats(s->history, s->history_path, &hs);
        s->history_loaded_ok = hs.loaded_ok;
        s->history_skipped_invalid = hs.skipped_invalid;
        history_trim_oldest(s->history, s->history_max_entries);
    }
    s->history_selected = 0;

    env_store_init(&s->envs);
    (void)env_store_load_file(
        &s->envs,
        s->paths.envs_json ? s->paths.envs_json : "config/envs.json"
    );

    s->header_suggestions = NULL;
    s->header_suggestions_count = 0;
    (void)header_suggestions_load(
        s->paths.headers_txt ? s->paths.headers_txt : "config/headers.txt",
        &s->header_suggestions,
        &s->header_suggestions_count
    );

    s->headers_ac_row = -1;
    s->headers_ac_next_match = 0;
    s->headers_ac_seed = NULL;

    s->prompt_kind = PROMPT_NONE;
    memset(s->prompt_input, 0, sizeof(s->prompt_input));
    s->prompt_len = 0;
    s->prompt_cursor = 0;
    memset(s->command_history, 0, sizeof(s->command_history));
    s->command_history_count = 0;
    s->command_history_index = -1;

    memset(s->search_query, 0, sizeof(s->search_query));
    s->search_target = SEARCH_TARGET_HISTORY;
    s->search_target_override = -1;
    s->search_match_index = -1;
    s->search_not_found = 0;

    s->body_scroll = 0;
    s->headers_scroll = 0;
}

void app_state_destroy(AppState *s) {
    for (;;) {
        app_state_lock(s);
        int inflight = s->is_request_in_flight;
        app_state_unlock(s);
        if (!inflight) break;
        usleep(1000);
    }

    tb_free(&s->body);
    tb_free(&s->headers);

    free(s->response.body);
    free(s->response.body_view);
    free(s->response.error);

    if (s->history) {
        history_free(s->history);
        free(s->history);
        s->history = NULL;
    }

    free(s->history_path);
    s->history_path = NULL;

    paths_free(&s->paths);

    layout_theme_catalog_free(&s->theme_catalog);
    s->active_theme_preset[0] = '\0';

    env_store_free(&s->envs);
    header_suggestions_free(s->header_suggestions, s->header_suggestions_count);
    s->header_suggestions = NULL;
    s->header_suggestions_count = 0;

    free(s->headers_ac_seed);
    s->headers_ac_seed = NULL;
    s->headers_ac_row = -1;
    s->headers_ac_next_match = 0;

    s->prompt_kind = PROMPT_NONE;
    s->prompt_len = 0;
    s->prompt_cursor = 0;
    s->command_history_count = 0;
    s->command_history_index = -1;
    s->search_target_override = -1;
    s->search_match_index = -1;
    s->search_not_found = 0;
    s->body_scroll = 0;
    s->headers_scroll = 0;
    (void)pthread_mutex_destroy(&s->state_mu);
}
