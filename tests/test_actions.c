#include "test.h"
#include "core/interaction/actions.h"
#include <string.h>

static int test_action_from_string_valid(void) {
    TEST_ASSERT(action_from_string("quit") == ACT_QUIT);
    TEST_ASSERT(action_from_string("move_down") == ACT_MOVE_DOWN);
    TEST_ASSERT(action_from_string("move_up") == ACT_MOVE_UP);
    TEST_ASSERT(action_from_string("send_request") == ACT_SEND_REQUEST);
    TEST_ASSERT(action_from_string("cycle_method") == ACT_CYCLE_METHOD);
    return 0;
}

static int test_action_from_string_invalid(void) {
    TEST_ASSERT(action_from_string(NULL) == ACT_NONE);
    TEST_ASSERT(action_from_string("") == ACT_NONE);
    TEST_ASSERT(action_from_string("invalid_action") == ACT_NONE);
    TEST_ASSERT(action_from_string("QUIT") == ACT_NONE);
    return 0;
}

static int test_action_to_string(void) {
    const char *s;
    
    s = action_to_string(ACT_QUIT);
    TEST_ASSERT(s != NULL && strcmp(s, "quit") == 0);
    
    s = action_to_string(ACT_SEND_REQUEST);
    TEST_ASSERT(s != NULL && strcmp(s, "send_request") == 0);
    
    s = action_to_string(ACT_NONE);
    TEST_ASSERT(s != NULL && strcmp(s, "none") == 0);
    
    s = action_to_string((Action)9999);
    TEST_ASSERT(s != NULL && strcmp(s, "none") == 0);
    return 0;
}

static int test_action_roundtrip(void) {
    Action actions[] = {
        ACT_QUIT, ACT_MOVE_DOWN, ACT_MOVE_UP, ACT_SEND_REQUEST,
        ACT_CYCLE_METHOD, ACT_ENTER_COMMAND, ACT_HISTORY_LOAD
    };
    
    for (unsigned i = 0; i < sizeof(actions)/sizeof(actions[0]); i++) {
        const char *str = action_to_string(actions[i]);
        Action parsed = action_from_string(str);
        TEST_ASSERT(parsed == actions[i]);
    }
    return 0;
}

static int test_action_description(void) {
    const char *desc;
    
    desc = action_description(ACT_QUIT, UI_LANG_EN);
    TEST_ASSERT(desc != NULL && strlen(desc) > 0);
    
    desc = action_description(ACT_SEND_REQUEST, UI_LANG_EN);
    TEST_ASSERT(desc != NULL && strlen(desc) > 0);
    
    desc = action_description(ACT_QUIT, UI_LANG_PT);
    TEST_ASSERT(desc != NULL && strlen(desc) > 0);
    
    desc = action_description((Action)9999, UI_LANG_EN);
    TEST_ASSERT(desc != NULL && strlen(desc) == 0);
    return 0;
}

int test_actions(void) {
    int failed = 0;
    failed += test_action_from_string_valid();
    failed += test_action_from_string_invalid();
    failed += test_action_to_string();
    failed += test_action_roundtrip();
    failed += test_action_description();
    
    if (failed) {
        printf("test_actions: FAILED (%d tests)\n", failed);
        return 1;
    }
    printf("test_actions: OK\n");
    return 0;
}
