#include "test.h"

#include "core/format.h"

int test_format(void) {
    char *pretty = json_pretty_print("{\"a\":1}");
    TEST_ASSERT(pretty != NULL);
    TEST_ASSERT(strstr(pretty, "\n") != NULL);
    free(pretty);

    pretty = json_pretty_print("{bad json");
    TEST_ASSERT(pretty == NULL);
    return 0;
}
