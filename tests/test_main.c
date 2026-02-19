#include <stdio.h>

int test_keymap(void);
int test_layout(void);
int test_env(void);
int test_history_storage(void);
int test_format(void);
int test_export_auth(void);
int test_i18n(void);
int test_utils(void);
int test_textbuf_navigation(void);

int main(void) {
    int rc = 0;

    rc |= test_keymap();
    rc |= test_layout();
    rc |= test_env();
    rc |= test_history_storage();
    rc |= test_format();
    rc |= test_export_auth();
    rc |= test_i18n();
    rc |= test_utils();
    rc |= test_textbuf_navigation();

    if (rc == 0) {
        printf("All tests passed.\n");
        return 0;
    }

    printf("Some tests failed.\n");
    return 1;
}
