#include "test.h"
#include "core/cli/help_builder.h"
#include "core/config/keymap.h"
#include "core/text/i18n.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Test: help_build_text with NULL keymap */
int test_help_build_null_keymap(void) {
    char *help = help_build_text(NULL, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "keymap") != NULL || strstr(help, "loaded") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text with empty keymap */
int test_help_build_empty_keymap(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strlen(help) > 0);
    
    /* Should contain basic commands */
    TEST_ASSERT(strstr(help, "quit") != NULL || strstr(help, "Quit") != NULL);
    TEST_ASSERT(strstr(help, "help") != NULL || strstr(help, "Help") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains basic commands section */
int test_help_contains_basic_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "BASIC") != NULL || strstr(help, "quit") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains language commands section */
int test_help_contains_lang_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "lang") != NULL || strstr(help, "Language") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains layout commands section */
int test_help_contains_layout_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "layout") != NULL || strstr(help, "Layout") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains theme commands section */
int test_help_contains_theme_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "theme") != NULL || strstr(help, "Theme") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains export commands section */
int test_help_contains_export_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "export") != NULL || strstr(help, "Export") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains auth commands section */
int test_help_contains_auth_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "auth") != NULL || strstr(help, "Auth") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains search commands section */
int test_help_contains_search_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "find") != NULL || strstr(help, "Search") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains history commands section */
int test_help_contains_history_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "clear") != NULL || strstr(help, "History") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text contains settings commands section */
int test_help_contains_settings_commands(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strstr(help, "set") != NULL || strstr(help, "Settings") != NULL);
    
    free(help);
    return 0;
}

/* Test: help_build_text with Portuguese language */
int test_help_build_portuguese(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    char *help = help_build_text(&km, UI_LANG_PT);
    
    TEST_ASSERT(help != NULL);
    TEST_ASSERT(strlen(help) > 0);
    
    free(help);
    return 0;
}

/* Test: help_build_text with key bindings */
int test_help_build_with_keybindings(void) {
    Keymap km;
    memset(&km, 0, sizeof(km));
    
    /* Add a simple keybinding */
    km.table[MODE_NORMAL]['q'] = ACT_QUIT;
    
    char *help = help_build_text(&km, UI_LANG_EN);
    
    TEST_ASSERT(help != NULL);
    /* Should contain the mode name and the key */
    TEST_ASSERT(strstr(help, "normal") != NULL || strstr(help, "Normal") != NULL);
    TEST_ASSERT(strstr(help, "q") != NULL);
    
    free(help);
    return 0;
}

/* Main test runner */
int test_help_builder(void) {
    int rc = 0;
    
    printf("Running test_help_builder...\n");
    rc |= test_help_build_null_keymap();
    rc |= test_help_build_empty_keymap();
    rc |= test_help_contains_basic_commands();
    rc |= test_help_contains_lang_commands();
    rc |= test_help_contains_layout_commands();
    rc |= test_help_contains_theme_commands();
    rc |= test_help_contains_export_commands();
    rc |= test_help_contains_auth_commands();
    rc |= test_help_contains_search_commands();
    rc |= test_help_contains_history_commands();
    rc |= test_help_contains_settings_commands();
    rc |= test_help_build_portuguese();
    rc |= test_help_build_with_keybindings();

    if (rc == 0) {
        printf("  test_help_builder: OK\n");
    } else {
        printf("  test_help_builder: FAILED\n");
    }

    return rc;
}
