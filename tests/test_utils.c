#include "test.h"
#include "core/utils.h"

#include <string.h>
#include <stdlib.h>

static int test_str_trim_whitespace(void) {
    char s1[] = "  hello  ";
    str_trim(s1);
    TEST_ASSERT_STR_EQ(s1, "hello");

    char s2[] = "\t\nworld\n\t";
    str_trim(s2);
    TEST_ASSERT_STR_EQ(s2, "world");

    char s3[] = "no_trim";
    str_trim(s3);
    TEST_ASSERT_STR_EQ(s3, "no_trim");

    char s4[] = "   ";
    str_trim(s4);
    TEST_ASSERT_STR_EQ(s4, "");

    char s5[] = "";
    str_trim(s5);
    TEST_ASSERT_STR_EQ(s5, "");

    return 0;
}

static int test_str_trim_left(void) {
    char *r1 = str_trim_left("  left");
    TEST_ASSERT_STR_EQ(r1, "left");

    char *r2 = str_trim_left("no_left_space");
    TEST_ASSERT_STR_EQ(r2, "no_left_space");

    char *r3 = str_trim_left("   ");
    TEST_ASSERT_STR_EQ(r3, "");

    return 0;
}

static int test_str_trim_right(void) {
    char s1[] = "right  ";
    str_trim_right(s1);
    TEST_ASSERT_STR_EQ(s1, "right");

    char s2[] = "no_right_space";
    str_trim_right(s2);
    TEST_ASSERT_STR_EQ(s2, "no_right_space");

    char s3[] = "   ";
    str_trim_right(s3);
    TEST_ASSERT_STR_EQ(s3, "");

    return 0;
}

static int test_str_eq_ci_basic(void) {
    TEST_ASSERT(str_eq_ci("hello", "HELLO"));
    TEST_ASSERT(str_eq_ci("World", "world"));
    TEST_ASSERT(str_eq_ci("TeSt", "tEsT"));
    TEST_ASSERT(str_eq_ci("same", "same"));

    TEST_ASSERT(!str_eq_ci("hello", "world"));
    TEST_ASSERT(!str_eq_ci("test", "testing"));
    TEST_ASSERT(!str_eq_ci("a", "b"));

    TEST_ASSERT(!str_eq_ci(NULL, "test"));
    TEST_ASSERT(!str_eq_ci("test", NULL));
    TEST_ASSERT(!str_eq_ci(NULL, NULL));

    return 0;
}

static int test_str_appendf_simple(void) {
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    TEST_ASSERT(str_appendf(&buf, &len, &cap, "Hello"));
    TEST_ASSERT(buf != NULL);
    TEST_ASSERT_STR_EQ(buf, "Hello");
    TEST_ASSERT(len == 5);
    TEST_ASSERT(cap >= 6);

    TEST_ASSERT(str_appendf(&buf, &len, &cap, " World"));
    TEST_ASSERT_STR_EQ(buf, "Hello World");
    TEST_ASSERT(len == 11);

    free(buf);
    return 0;
}

static int test_str_appendf_formatting(void) {
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    TEST_ASSERT(str_appendf(&buf, &len, &cap, "Number: %d", 42));
    TEST_ASSERT_STR_EQ(buf, "Number: 42");

    TEST_ASSERT(str_appendf(&buf, &len, &cap, ", String: %s", "test"));
    TEST_ASSERT_STR_EQ(buf, "Number: 42, String: test");

    TEST_ASSERT(str_appendf(&buf, &len, &cap, ", Float: %.1f", 3.14));
    TEST_ASSERT_STR_EQ(buf, "Number: 42, String: test, Float: 3.1");

    free(buf);
    return 0;
}

static int test_str_appendf_growth(void) {
    char *buf = NULL;
    size_t len = 0;
    size_t cap = 0;

    /* Append many times to test buffer growth */
    for (int i = 0; i < 100; i++) {
        TEST_ASSERT(str_appendf(&buf, &len, &cap, "x"));
    }
    TEST_ASSERT(len == 100);
    TEST_ASSERT(cap >= 101);

    free(buf);
    return 0;
}

int test_utils(void) {
    int rc = 0;
    
    printf("Running test_utils...\n");
    rc |= test_str_trim_whitespace();
    rc |= test_str_trim_left();
    rc |= test_str_trim_right();
    rc |= test_str_eq_ci_basic();
    rc |= test_str_appendf_simple();
    rc |= test_str_appendf_formatting();
    rc |= test_str_appendf_growth();

    if (rc == 0) {
        printf("  test_utils: OK\n");
    } else {
        printf("  test_utils: FAILED\n");
    }

    return rc;
}
