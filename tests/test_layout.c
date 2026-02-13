#include "test.h"

#include "core/layout.h"

int test_layout(void) {
    const char *path = "tests/fixtures/layout_test.conf";

    LayoutProfile profile;
    LayoutSlot hs, es, rs;
    LayoutSizing sizing;
    LayoutTheme theme;
    char preset[64];
    TEST_ASSERT(layout_load_config(path, &profile, &hs, &es, &rs, &sizing, &theme, preset, sizeof(preset)) == 0);
    TEST_ASSERT(profile == LAYOUT_PROFILE_QUAD);
    TEST_ASSERT(hs == LAYOUT_SLOT_TL);
    TEST_ASSERT(es == LAYOUT_SLOT_TR);
    TEST_ASSERT(rs == LAYOUT_SLOT_BR);
    TEST_ASSERT_STR_EQ(preset, "vivid");
    return 0;
}
