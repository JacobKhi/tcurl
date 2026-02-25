#include "state.h"
#include <string.h>
#include <stdlib.h>
#include "core/storage/history.h"
#include "core/storage/history_persistence.h"
#include "core/config/env.h"
#include "core/config/layout.h"
#include "core/storage/paths.h"
#include "core/text/textbuf.h"
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

    /* Initialize Config State */
    paths_init(&s->config.paths);
    if (paths_resolve_config_dir(&s->config.paths) != 0 || paths_build_file_paths(&s->config.paths) != 0) {
        paths_free(&s->config.paths);
        paths_init(&s->config.paths);
    }

    s->running = 1;

    /* Initialize UI State */
    s->ui.mode = MODE_NORMAL;
    s->ui.focused_panel = PANEL_HISTORY;
    s->ui.layout_profile = LAYOUT_PROFILE_CLASSIC;
    s->ui.quad_history_slot = LAYOUT_SLOT_TL;
    s->ui.quad_editor_slot = LAYOUT_SLOT_TR;
    s->ui.quad_response_slot = LAYOUT_SLOT_BR;
    memset(s->ui.active_theme_preset, 0, sizeof(s->ui.active_theme_preset));
    (void)layout_load_config(
        s->config.paths.layout_conf_load ? s->config.paths.layout_conf_load : "config/layout.conf",
        &s->ui.layout_profile,
        &s->ui.quad_history_slot,
        &s->ui.quad_editor_slot,
        &s->ui.quad_response_slot,
        &s->ui.layout_sizing,
        &s->ui.theme,
        &s->ui.show_footer_hint,
        &s->ui.language_setting,
        s->ui.active_theme_preset,
        sizeof(s->ui.active_theme_preset)
    );
    s->ui.language = i18n_resolve_language(s->ui.language_setting, getenv("LANG"));
    layout_theme_catalog_init(&s->ui.theme_catalog);
    (void)layout_theme_catalog_load(
        s->config.paths.themes_conf ? s->config.paths.themes_conf : "config/themes.conf",
        &s->ui.theme_catalog
    );
    if (s->ui.active_theme_preset[0] != '\0') {
        LayoutTheme themed;
        if (layout_theme_catalog_apply(&s->ui.theme_catalog, s->ui.active_theme_preset, &themed) == 0) {
            s->ui.theme = themed;
        } else {
            s->ui.active_theme_preset[0] = '\0';
        }
    }

    /* Initialize Editor State */
    memset(s->editor.url, 0, sizeof(s->editor.url));
    s->editor.url_len = 0;
    s->editor.url_cursor = 0;
    tb_init(&s->editor.body);
    tb_init(&s->editor.headers);
    s->editor.active_field = EDIT_FIELD_URL;
    s->editor.method = HTTP_GET;
    s->editor.body_scroll = 0;
    s->editor.headers_scroll = 0;

    /* Initialize Response State */
    s->response.response.status = 0;
    s->response.response.body = NULL;
    s->response.response.body_view = NULL;
    s->response.response.elapsed_ms = 0.0;
    s->response.response.error = NULL;
    s->response.response.is_json = 0;
    s->response.is_request_in_flight = 0;
    s->response.scroll = 0;

    /* Initialize History State */
    s->history.max_entries = history_config_load_max_entries(
        s->config.paths.history_conf ? s->config.paths.history_conf : "config/history.conf",
        500
    );
    s->history.path = history_storage_default_path();
    if (!s->history.path) {
        s->history.path = strdup("./history.jsonl");
    }

    s->history.loaded_ok = 0;
    s->history.skipped_invalid = 0;
    s->history.last_save_error = 0;
    s->history.history = malloc(sizeof(*s->history.history));
    if (s->history.history) {
        HistoryLoadStats hs = {0};
        history_init(s->history.history);
        (void)history_storage_load_with_stats(s->history.history, s->history.path, &hs);
        s->history.loaded_ok = hs.loaded_ok;
        s->history.skipped_invalid = hs.skipped_invalid;
        history_trim_oldest(s->history.history, s->history.max_entries);
    }
    s->history.selected = 0;

    /* Initialize Config State - Environments and Suggestions */
    env_store_init(&s->config.envs);
    (void)env_store_load_file(
        &s->config.envs,
        s->config.paths.envs_json ? s->config.paths.envs_json : "config/envs.json"
    );

    s->config.header_suggestions = NULL;
    s->config.header_suggestions_count = 0;
    (void)header_suggestions_load(
        s->config.paths.headers_txt ? s->config.paths.headers_txt : "config/headers.txt",
        &s->config.header_suggestions,
        &s->config.header_suggestions_count
    );

    s->config.headers_ac_row = -1;
    s->config.headers_ac_next_match = 0;
    s->config.headers_ac_seed = NULL;

    /* Initialize Prompt State */
    s->prompt.kind = PROMPT_NONE;
    memset(s->prompt.input, 0, sizeof(s->prompt.input));
    s->prompt.len = 0;
    s->prompt.cursor = 0;
    memset(s->prompt.command_history, 0, sizeof(s->prompt.command_history));
    s->prompt.command_history_count = 0;
    s->prompt.command_history_index = -1;

    /* Initialize Search State */
    memset(s->search.query, 0, sizeof(s->search.query));
    s->search.target = SEARCH_TARGET_HISTORY;
    s->search.target_override = -1;
    s->search.match_index = -1;
    s->search.not_found = 0;
}

void app_state_destroy(AppState *s) {
    /* Wait for any in-flight requests to complete */
    for (;;) {
        app_state_lock(s);
        int inflight = s->response.is_request_in_flight;
        app_state_unlock(s);
        if (!inflight) break;
        usleep(1000);
    }

    /* Destroy Editor State */
    tb_free(&s->editor.body);
    tb_free(&s->editor.headers);
    s->editor.body_scroll = 0;
    s->editor.headers_scroll = 0;

    /* Destroy Response State */
    free(s->response.response.body);
    free(s->response.response.body_view);
    free(s->response.response.response_headers);
    free(s->response.response.error);

    /* Destroy History State */
    if (s->history.history) {
        history_free(s->history.history);
        free(s->history.history);
        s->history.history = NULL;
    }
    free(s->history.path);
    s->history.path = NULL;

    /* Destroy UI State */
    layout_theme_catalog_free(&s->ui.theme_catalog);
    s->ui.active_theme_preset[0] = '\0';

    /* Destroy Config State */
    paths_free(&s->config.paths);
    env_store_free(&s->config.envs);
    header_suggestions_free(s->config.header_suggestions, s->config.header_suggestions_count);
    s->config.header_suggestions = NULL;
    s->config.header_suggestions_count = 0;

    free(s->config.headers_ac_seed);
    s->config.headers_ac_seed = NULL;
    s->config.headers_ac_row = -1;
    s->config.headers_ac_next_match = 0;

    /* Reset Prompt State */
    s->prompt.kind = PROMPT_NONE;
    s->prompt.len = 0;
    s->prompt.cursor = 0;
    s->prompt.command_history_count = 0;
    s->prompt.command_history_index = -1;

    /* Reset Search State */
    s->search.target_override = -1;
    s->search.match_index = -1;
    s->search.not_found = 0;

    (void)pthread_mutex_destroy(&s->state_mu);
}
