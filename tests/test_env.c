#include "test.h"

#include "core/env.h"

int test_env(void) {
    const char *path = "tests/fixtures/env_test.json";

    EnvStore store;
    env_store_init(&store);
    TEST_ASSERT(env_store_load_file(&store, path) == 0);
    TEST_ASSERT_STR_EQ(env_store_active_name(&store), "dev");
    TEST_ASSERT_STR_EQ(env_store_lookup(&store, "TOKEN"), "abc");

    char *missing = NULL;
    char *expanded = env_expand_template(&store, "{{BASE_URL}}/anything", &missing);
    TEST_ASSERT(expanded != NULL);
    TEST_ASSERT_STR_EQ(expanded, "https://httpbin.org/anything");
    free(expanded);
    TEST_ASSERT(missing == NULL);

    expanded = env_expand_template(&store, "{{MISSING}}", &missing);
    TEST_ASSERT(expanded == NULL);
    TEST_ASSERT(missing != NULL);
    free(missing);

    env_store_free(&store);
    return 0;
}
