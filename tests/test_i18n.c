#include "test.h"

#include "core/i18n.h"

int test_i18n(void) {
    TEST_ASSERT(i18n_resolve_language(UI_LANG_SETTING_PT, "en_US.UTF-8") == UI_LANG_PT);
    TEST_ASSERT(i18n_resolve_language(UI_LANG_SETTING_EN, "pt_BR.UTF-8") == UI_LANG_EN);

    TEST_ASSERT(i18n_resolve_language(UI_LANG_SETTING_AUTO, "pt_BR.UTF-8") == UI_LANG_PT);
    TEST_ASSERT(i18n_resolve_language(UI_LANG_SETTING_AUTO, "pt-PT") == UI_LANG_PT);
    TEST_ASSERT(i18n_resolve_language(UI_LANG_SETTING_AUTO, "en_US.UTF-8") == UI_LANG_EN);
    TEST_ASSERT(i18n_resolve_language(UI_LANG_SETTING_AUTO, "") == UI_LANG_EN);
    TEST_ASSERT(i18n_resolve_language(UI_LANG_SETTING_AUTO, NULL) == UI_LANG_EN);

    TEST_ASSERT_STR_EQ(i18n_get(UI_LANG_EN, I18N_SENDING_REQUEST), "Sending request...");
    TEST_ASSERT_STR_EQ(i18n_get(UI_LANG_PT, I18N_SENDING_REQUEST), "Enviando requisição...");

    TEST_ASSERT_STR_EQ(i18n_get(UI_LANG_PT, (I18nKey)999), "UNKNOWN");

    {
        UiLanguageSetting s = UI_LANG_SETTING_AUTO;
        TEST_ASSERT(i18n_parse_language_setting("auto", &s) == 0);
        TEST_ASSERT(s == UI_LANG_SETTING_AUTO);
        TEST_ASSERT(i18n_parse_language_setting("en", &s) == 0);
        TEST_ASSERT(s == UI_LANG_SETTING_EN);
        TEST_ASSERT(i18n_parse_language_setting("pt", &s) == 0);
        TEST_ASSERT(s == UI_LANG_SETTING_PT);
        TEST_ASSERT(i18n_parse_language_setting("xx", &s) != 0);
    }

    TEST_ASSERT_STR_EQ(i18n_language_setting_name(UI_LANG_SETTING_AUTO), "auto");
    TEST_ASSERT_STR_EQ(i18n_language_setting_name(UI_LANG_SETTING_EN), "en");
    TEST_ASSERT_STR_EQ(i18n_language_setting_name(UI_LANG_SETTING_PT), "pt");

    return 0;
}
