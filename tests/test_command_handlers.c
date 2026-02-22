#include "test.h"
#include "core/cli/command_handlers.h"
#include "core/storage/history.h"
#include "core/config/layout.h"
#include "core/interaction/auth.h"
#include "core/text/i18n.h"
#include "state.h"
#include <string.h>
#include <stdlib.h>

/* Helper: initialize minimal AppState for testing */
static void init_minimal_state(AppState *s) {
    memset(s, 0, sizeof(AppState));
    s->ui.language = UI_LANG_EN;
    s->ui.language_setting = UI_LANG_SETTING_EN;
    s->ui.layout_profile = LAYOUT_PROFILE_CLASSIC;
    s->history.max_entries = 100;
    s->search.target_override = -1;
    
    tb_init(&s->editor.body);
    tb_init(&s->editor.headers);
    
    s->history.history = malloc(sizeof(History));
    if (s->history.history) {
        history_init(s->history.history);
    }
    
    layout_theme_catalog_init(&s->ui.theme_catalog);
}

/* Helper: cleanup test AppState */
static void cleanup_state(AppState *s) {
    if (s->history.history) {
        history_free(s->history.history);
        free(s->history.history);
    }
    
    free(s->response.response.body);
    free(s->response.response.body_view);
    free(s->response.response.error);
    
    tb_free(&s->editor.body);
    tb_free(&s->editor.headers);
    
    layout_theme_catalog_free(&s->ui.theme_catalog);
}

/* Test: cmd_apply_theme with invalid preset */
int test_cmd_apply_theme_invalid(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_apply_theme(&s, "nonexistent_theme_xyz", 0);
    
    TEST_ASSERT(s.response.response.error != NULL);
    TEST_ASSERT(strstr(s.response.response.error, "Unknown") != NULL || 
                strstr(s.response.response.error, "theme") != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_apply_theme with empty name */
int test_cmd_apply_theme_empty(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_apply_theme(&s, "", 0);
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_list_themes */
int test_cmd_list_themes(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_list_themes(&s);
    
    TEST_ASSERT(s.response.response.body != NULL);
    TEST_ASSERT(strlen(s.response.response.body) > 0);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_clear_history */
int test_cmd_clear_history(void) {
    AppState s;
    init_minimal_state(&s);
    
    /* Add some history items */
    TextBuffer body, headers;
    tb_init(&body);
    tb_init(&headers);
    
    tb_set_from_string(&body, "test body");
    tb_set_from_string(&headers, "Content-Type: text/plain");
    
    HttpResponse response;
    memset(&response, 0, sizeof(response));
    response.status = 200;
    response.elapsed_ms = 100.0;
    response.body = strdup("response body");
    response.body_view = strdup("response body");
    
    history_push(s.history.history, HTTP_GET, "http://example.com", &body, &headers, &response);
    
    free(response.body);
    free(response.body_view);
    tb_free(&body);
    tb_free(&headers);
    
    TEST_ASSERT(s.history.history->count == 1);
    
    cmd_clear_history(&s);
    
    TEST_ASSERT(s.history.history->count == 0);
    TEST_ASSERT(s.response.response.body != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_export_request with invalid format */
int test_cmd_export_invalid_format(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_export_request(&s, "invalid_format");
    
    TEST_ASSERT(s.response.response.error != NULL);
    TEST_ASSERT(strstr(s.response.response.error, "format") != NULL || 
                strstr(s.response.response.error, "Unknown") != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_export_request with empty format */
int test_cmd_export_empty_format(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_export_request(&s, "");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_auth with bearer token */
int test_cmd_auth_bearer(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_auth(&s, "bearer", "test_token_123");
    
    /* Should set response body on success */
    TEST_ASSERT(s.response.response.body != NULL || s.response.response.error != NULL);
    
    /* Check if Authorization header was added */
    char *headers_str = tb_to_string(&s.editor.headers);
    if (headers_str) {
        int found = strstr(headers_str, "Authorization:") != NULL;
        free(headers_str);
        TEST_ASSERT(found == 1);
    }
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_auth with basic auth */
int test_cmd_auth_basic(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_auth(&s, "basic", "user:pass");
    
    /* Should set response body on success */
    TEST_ASSERT(s.response.response.body != NULL || s.response.response.error != NULL);
    
    /* Check if Authorization header was added */
    char *headers_str = tb_to_string(&s.editor.headers);
    if (headers_str) {
        int found = strstr(headers_str, "Authorization:") != NULL;
        free(headers_str);
        TEST_ASSERT(found == 1);
    }
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_auth with invalid kind */
int test_cmd_auth_invalid_kind(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_auth(&s, "invalid_auth", "token");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_auth with missing arguments */
int test_cmd_auth_missing_args(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_auth(&s, "bearer", "");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_find with empty query */
int test_cmd_find_empty(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_find(&s, "");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_set without arguments (display settings) */
int test_cmd_set_no_args(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_set(&s, NULL, NULL);
    
    TEST_ASSERT(s.response.response.body != NULL);
    TEST_ASSERT(strstr(s.response.response.body, "auto") != NULL || 
                strstr(s.response.response.body, "100") != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_set with search_target */
int test_cmd_set_search_target(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_set(&s, "search_target", "history");
    
    TEST_ASSERT(s.search.target_override == SEARCH_TARGET_HISTORY);
    TEST_ASSERT(s.response.response.body != NULL);
    
    cmd_set(&s, "search_target", "response");
    
    TEST_ASSERT(s.search.target_override == SEARCH_TARGET_RESPONSE);
    
    cmd_set(&s, "search_target", "auto");
    
    TEST_ASSERT(s.search.target_override == -1);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_set with max_entries */
int test_cmd_set_max_entries(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_set(&s, "max_entries", "50");
    
    TEST_ASSERT(s.history.max_entries == 50);
    TEST_ASSERT(s.response.response.body != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_set with invalid setting */
int test_cmd_set_invalid_setting(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_set(&s, "invalid_setting", "value");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_lang with list */
int test_cmd_lang_list(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_lang(&s, "list");
    
    TEST_ASSERT(s.response.response.body != NULL);
    TEST_ASSERT(strstr(s.response.response.body, "en") != NULL);
    TEST_ASSERT(strstr(s.response.response.body, "pt") != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_lang with valid language */
int test_cmd_lang_set(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_lang(&s, "en");
    
    TEST_ASSERT(s.ui.language_setting == UI_LANG_SETTING_EN);
    TEST_ASSERT(s.response.response.body != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_lang with empty argument */
int test_cmd_lang_empty(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_lang(&s, "");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_layout with list */
int test_cmd_layout_list(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_layout(&s, "list");
    
    TEST_ASSERT(s.response.response.body != NULL);
    TEST_ASSERT(strstr(s.response.response.body, "classic") != NULL);
    TEST_ASSERT(strstr(s.response.response.body, "quad") != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_layout with valid layout */
int test_cmd_layout_set(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_layout(&s, "quad");
    
    TEST_ASSERT(s.ui.layout_profile == LAYOUT_PROFILE_QUAD);
    TEST_ASSERT(s.response.response.body != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_layout with invalid layout */
int test_cmd_layout_invalid(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_layout(&s, "invalid_layout");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Test: cmd_layout with empty argument */
int test_cmd_layout_empty(void) {
    AppState s;
    init_minimal_state(&s);
    
    cmd_layout(&s, "");
    
    TEST_ASSERT(s.response.response.error != NULL);
    
    cleanup_state(&s);
    return 0;
}

/* Main test runner */
int test_command_handlers(void) {
    int rc = 0;
    
    printf("Running test_command_handlers...\n");
    rc |= test_cmd_apply_theme_invalid();
    rc |= test_cmd_apply_theme_empty();
    rc |= test_cmd_list_themes();
    rc |= test_cmd_clear_history();
    rc |= test_cmd_export_invalid_format();
    rc |= test_cmd_export_empty_format();
    rc |= test_cmd_auth_bearer();
    rc |= test_cmd_auth_basic();
    rc |= test_cmd_auth_invalid_kind();
    rc |= test_cmd_auth_missing_args();
    rc |= test_cmd_find_empty();
    rc |= test_cmd_set_no_args();
    rc |= test_cmd_set_search_target();
    rc |= test_cmd_set_max_entries();
    rc |= test_cmd_set_invalid_setting();
    rc |= test_cmd_lang_list();
    rc |= test_cmd_lang_set();
    rc |= test_cmd_lang_empty();
    rc |= test_cmd_layout_list();
    rc |= test_cmd_layout_set();
    rc |= test_cmd_layout_invalid();
    rc |= test_cmd_layout_empty();

    if (rc == 0) {
        printf("  test_command_handlers: OK\n");
    } else {
        printf("  test_command_handlers: FAILED\n");
    }

    return rc;
}
