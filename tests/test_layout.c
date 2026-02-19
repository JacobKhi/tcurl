#include "test.h"

#include "core/layout.h"

static int file_contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }

    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return 0;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return 0;
    }

    size_t nread = fread(buf, 1, (size_t)size, f);
    fclose(f);
    buf[nread] = '\0';

    int ok = strstr(buf, needle) != NULL;
    free(buf);
    return ok;
}

int test_layout(void) {
    const char *fixture_path = "tests/fixtures/layout_test.conf";
    const char *tmp_path = "/tmp/tcurl_test_layout.conf";
    const char *saved_path = "/tmp/tcurl_test_layout_saved.conf";

    LayoutProfile profile;
    LayoutSlot hs, es, rs;
    LayoutSizing sizing;
    LayoutTheme theme;
    int show_footer_hint = 0;
    UiLanguageSetting language_setting = UI_LANG_SETTING_AUTO;
    char preset[64];
    TEST_ASSERT(
        layout_load_config(
            fixture_path,
            &profile,
            &hs,
            &es,
            &rs,
            &sizing,
            &theme,
            &show_footer_hint,
            &language_setting,
            preset,
            sizeof(preset)
        ) == 0
    );
    TEST_ASSERT(profile == LAYOUT_PROFILE_QUAD);
    TEST_ASSERT(hs == LAYOUT_SLOT_TL);
    TEST_ASSERT(es == LAYOUT_SLOT_TR);
    TEST_ASSERT(rs == LAYOUT_SLOT_BR);
    TEST_ASSERT(show_footer_hint == 0);
    TEST_ASSERT(language_setting == UI_LANG_SETTING_PT);
    TEST_ASSERT_STR_EQ(preset, "vivid");

    TEST_ASSERT(
        layout_save_config(
            saved_path,
            profile,
            hs,
            es,
            rs,
            &sizing,
            &theme,
            show_footer_hint,
            UI_LANG_SETTING_PT,
            preset
        ) == 0
    );
    TEST_ASSERT(file_contains(saved_path, "language = pt"));

    TEST_ASSERT(
        write_text_file(
            tmp_path,
            "profile = classic\n"
            "language = en\n"
        ) == 0
    );
    TEST_ASSERT(
        layout_load_config(
            tmp_path,
            &profile,
            &hs,
            &es,
            &rs,
            &sizing,
            &theme,
            &show_footer_hint,
            &language_setting,
            preset,
            sizeof(preset)
        ) == 0
    );
    TEST_ASSERT(language_setting == UI_LANG_SETTING_EN);

    TEST_ASSERT(
        write_text_file(
            tmp_path,
            "profile = classic\n"
            "language = invalid\n"
        ) == 0
    );
    TEST_ASSERT(
        layout_load_config(
            tmp_path,
            &profile,
            &hs,
            &es,
            &rs,
            &sizing,
            &theme,
            &show_footer_hint,
            &language_setting,
            preset,
            sizeof(preset)
        ) == 0
    );
    TEST_ASSERT(language_setting == UI_LANG_SETTING_AUTO);

    TEST_ASSERT(
        write_text_file(
            tmp_path,
            "profile = classic\n"
        ) == 0
    );
    TEST_ASSERT(
        layout_load_config(
            tmp_path,
            &profile,
            &hs,
            &es,
            &rs,
            &sizing,
            &theme,
            &show_footer_hint,
            &language_setting,
            preset,
            sizeof(preset)
        ) == 0
    );
    TEST_ASSERT(language_setting == UI_LANG_SETTING_AUTO);

    return 0;
}
