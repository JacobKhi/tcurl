#include "test.h"

#include "core/actions.h"
#include "core/keymap.h"
#include <ncurses.h>

int test_keymap(void) {
    const char *path = "tests/fixtures/keymap_test.conf";

    Keymap km;
    TEST_ASSERT(keymap_load_file(&km, path) == 0);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, 'q') == ACT_QUIT);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, KEY_ENTER) == ACT_HISTORY_LOAD);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, '\n') == ACT_HISTORY_LOAD);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, 'n') == ACT_SEARCH_NEXT);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, KEY_LEFT) == ACT_FOCUS_LEFT);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, KEY_RIGHT) == ACT_FOCUS_RIGHT);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, KEY_UP) == ACT_MOVE_UP);
    TEST_ASSERT(keymap_resolve(&km, MODE_NORMAL, KEY_DOWN) == ACT_MOVE_DOWN);

    return 0;
}
