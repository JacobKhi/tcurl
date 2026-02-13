#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "ASSERT FAILED: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

#define TEST_ASSERT_STR_EQ(a, b) \
    do { \
        if (strcmp((a), (b)) != 0) { \
            fprintf(stderr, "ASSERT STR FAILED: '%s' != '%s' (%s:%d)\n", (a), (b), __FILE__, __LINE__); \
            return 1; \
        } \
    } while (0)

static inline int write_text_file(const char *path, const char *content) {
    FILE *f = fopen(path, "w");
    if (!f) return 1;
    if (fputs(content, f) < 0) {
        fclose(f);
        return 1;
    }
    if (fclose(f) != 0) return 1;
    return 0;
}
