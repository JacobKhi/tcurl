#include "test.h"
#include "core/interaction/search.h"
#include "core/storage/history.h"
#include "core/text/textbuf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Test search_get_effective_target
static int test_search_effective_target(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    // Default: based on focused panel
    s.search.target_override = -1;
    s.ui.focused_panel = PANEL_HISTORY;
    TEST_ASSERT(search_get_effective_target(&s) == SEARCH_TARGET_HISTORY);
    
    s.ui.focused_panel = PANEL_RESPONSE;
    TEST_ASSERT(search_get_effective_target(&s) == SEARCH_TARGET_RESPONSE);
    
    // Override to history
    s.search.target_override = SEARCH_TARGET_HISTORY;
    s.ui.focused_panel = PANEL_RESPONSE;
    TEST_ASSERT(search_get_effective_target(&s) == SEARCH_TARGET_HISTORY);
    
    // Override to response
    s.search.target_override = SEARCH_TARGET_RESPONSE;
    s.ui.focused_panel = PANEL_HISTORY;
    TEST_ASSERT(search_get_effective_target(&s) == SEARCH_TARGET_RESPONSE);

    return 0;
}

// Test search_apply with empty history
static int test_search_apply_empty_history(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    
    search_apply(&s, "test");
    
    TEST_ASSERT(s.search.not_found == 1);
    TEST_ASSERT(s.search.match_index == -1);
    
    history_free(&h);
    return 0;
}

// Test search_apply with history matches
static int test_search_apply_history_match(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    
    // Add some items using history_push
    TextBuffer empty_tb;
    tb_init(&empty_tb);
    
    history_push(&h, HTTP_GET, "https://api.example.com/users", &empty_tb, &empty_tb, NULL);
    history_push(&h, HTTP_POST, "https://api.example.com/posts", &empty_tb, &empty_tb, NULL);
    history_push(&h, HTTP_GET, "https://other.com/data", &empty_tb, &empty_tb, NULL);
    
    tb_free(&empty_tb);
    
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    
    // Search for "users" - should match first item
    search_apply(&s, "users");
    
    TEST_ASSERT(s.search.not_found == 0);
    TEST_ASSERT(strcmp(s.search.query, "users") == 0);
    TEST_ASSERT(s.history.selected == 0);
    TEST_ASSERT(s.search.match_index == 0);
    
    history_free(&h);
    return 0;
}

// Test search_apply case insensitive
static int test_search_apply_case_insensitive(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    
    TextBuffer empty_tb;
    tb_init(&empty_tb);
    
    history_push(&h, HTTP_GET, "https://API.EXAMPLE.COM/Users", &empty_tb, &empty_tb, NULL);
    
    tb_free(&empty_tb);
    
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    
    // Search lowercase - should match uppercase
    search_apply(&s, "users");
    TEST_ASSERT(s.search.not_found == 0);
    
    // Search uppercase - should match
    search_apply(&s, "API");
    TEST_ASSERT(s.search.not_found == 0);
    
    // Search mixed case - should match
    search_apply(&s, "ExAmPlE");
    TEST_ASSERT(s.search.not_found == 0);
    
    history_free(&h);
    return 0;
}

// Test search_apply with no matches
static int test_search_apply_no_match(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    
    TextBuffer empty_tb;
    tb_init(&empty_tb);
    
    history_push(&h, HTTP_GET, "https://api.example.com/users", &empty_tb, &empty_tb, NULL);
    
    tb_free(&empty_tb);
    
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    
    // Search for something that doesn't exist
    search_apply(&s, "notfound");
    
    TEST_ASSERT(s.search.not_found == 1);
    TEST_ASSERT(s.search.match_index == -1);
    
    history_free(&h);
    return 0;
}

// Test search_apply with response body
static int test_search_apply_response(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    s.response.response.body_view = strdup("Line 1: Hello\nLine 2: World\nLine 3: Test\nLine 4: Hello again");
    s.search.target = SEARCH_TARGET_RESPONSE;
    
    // Search for "Hello" - should find first occurrence (line 0)
    search_apply(&s, "Hello");
    
    TEST_ASSERT(s.search.not_found == 0);
    TEST_ASSERT(strcmp(s.search.query, "Hello") == 0);
    TEST_ASSERT(s.response.scroll == 0);
    TEST_ASSERT(s.search.match_index == 0);
    
    free(s.response.response.body_view);
    return 0;
}

// Test search_apply with empty query
static int test_search_apply_empty_query(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    
    search_apply(&s, "");
    
    // Empty query should set query but not mark as not found
    TEST_ASSERT(strcmp(s.search.query, "") == 0);
    TEST_ASSERT(s.search.match_index == -1);
    
    history_free(&h);
    return 0;
}

// Test search_step forward in history
static int test_search_step_history_forward(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    
    TextBuffer empty_tb;
    tb_init(&empty_tb);
    
    // Add items with "test" in them
    for (int i = 0; i < 5; i++) {
        char url[256];
        if (i % 2 == 0) {
            snprintf(url, sizeof(url), "https://api.example.com/test%d", i);
        } else {
            snprintf(url, sizeof(url), "https://api.example.com/other%d", i);
        }
        history_push(&h, HTTP_GET, url, &empty_tb, &empty_tb, NULL);
    }
    
    tb_free(&empty_tb);
    
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    
    // Apply search first
    search_apply(&s, "test");
    TEST_ASSERT(s.search.not_found == 0);
    TEST_ASSERT(s.history.selected == 0); // First match
    
    // Step forward
    search_step(&s, +1);
    TEST_ASSERT(s.history.selected == 2); // Next match
    
    // Step forward again
    search_step(&s, +1);
    TEST_ASSERT(s.history.selected == 4); // Next match
    
    // Step forward again - should wrap to first
    search_step(&s, +1);
    TEST_ASSERT(s.history.selected == 0); // Wrap around
    
    history_free(&h);
    return 0;
}

// Test search_step backward in history
static int test_search_step_history_backward(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    
    TextBuffer empty_tb;
    tb_init(&empty_tb);
    
    // Add items with "test" in them
    for (int i = 0; i < 5; i++) {
        char url[256];
        if (i % 2 == 0) {
            snprintf(url, sizeof(url), "https://api.example.com/test%d", i);
        } else {
            snprintf(url, sizeof(url), "https://api.example.com/other%d", i);
        }
        history_push(&h, HTTP_GET, url, &empty_tb, &empty_tb, NULL);
    }
    
    tb_free(&empty_tb);
    
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    
    // Apply search first
    search_apply(&s, "test");
    TEST_ASSERT(s.search.not_found == 0);
    TEST_ASSERT(s.history.selected == 0); // First match
    
    // Step backward - should wrap to last
    search_step(&s, -1);
    TEST_ASSERT(s.history.selected == 4); // Last match
    
    // Step backward again
    search_step(&s, -1);
    TEST_ASSERT(s.history.selected == 2); // Previous match
    
    history_free(&h);
    return 0;
}

// Test search_step with response
static int test_search_step_response(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    s.response.response.body_view = strdup("Line 0: test\nLine 1: other\nLine 2: test\nLine 3: test");
    s.search.target = SEARCH_TARGET_RESPONSE;
    
    // Apply search first
    search_apply(&s, "test");
    TEST_ASSERT(s.response.scroll == 0); // First match (line 0)
    
    // Step forward
    search_step(&s, +1);
    TEST_ASSERT(s.response.scroll == 2); // Next match (line 2)
    
    // Step forward again
    search_step(&s, +1);
    TEST_ASSERT(s.response.scroll == 3); // Next match (line 3)
    
    // Step forward again - wrap to first
    search_step(&s, +1);
    TEST_ASSERT(s.response.scroll == 0); // Wrap around
    
    // Step backward
    search_step(&s, -1);
    TEST_ASSERT(s.response.scroll == 3); // Last match
    
    free(s.response.response.body_view);
    return 0;
}

// Test search_step with no query
static int test_search_step_no_query(void) {
    AppState s;
    memset(&s, 0, sizeof(s));
    
    History h;
    history_init(&h);
    s.history.history = &h;
    s.search.target = SEARCH_TARGET_HISTORY;
    s.search.query[0] = '\0'; // Empty query
    
    int old_selected = s.history.selected;
    
    // Step should do nothing with empty query
    search_step(&s, +1);
    TEST_ASSERT(s.history.selected == old_selected);
    
    history_free(&h);
    return 0;
}

int test_search(void) {
    int rc = 0;
    
    printf("Running test_search...\n");
    rc |= test_search_effective_target();
    rc |= test_search_apply_empty_history();
    rc |= test_search_apply_history_match();
    rc |= test_search_apply_case_insensitive();
    rc |= test_search_apply_no_match();
    rc |= test_search_apply_response();
    rc |= test_search_apply_empty_query();
    rc |= test_search_step_history_forward();
    rc |= test_search_step_history_backward();
    rc |= test_search_step_response();
    rc |= test_search_step_no_query();

    if (rc == 0) {
        printf("  test_search: OK\n");
    } else {
        printf("  test_search: FAILED\n");
    }

    return rc;
}
