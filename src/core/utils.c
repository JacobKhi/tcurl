#include "core/utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void str_trim(char *s) {
    if (!s) return;
    
    /* Trim leading whitespace by moving content */
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
    
    /* Trim trailing whitespace by adding null terminators */
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
}

char *str_trim_left(char *s) {
    if (!s) return NULL;
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

void str_trim_right(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
}

int str_eq_ci(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

int str_appendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...) {
    if (!buf || !len || !cap || !fmt) return 0;
    
    /* Calculate required size for formatted string */
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (need < 0) return 0;
    
    /* Ensure buffer has enough capacity */
    size_t req = *len + (size_t)need + 1;
    if (*cap < req) {
        size_t new_cap = *cap ? *cap : 256;
        while (new_cap < req) new_cap *= 2;
        char *n = realloc(*buf, new_cap);
        if (!n) return 0;
        *buf = n;
        *cap = new_cap;
    }
    
    /* Append formatted string to buffer */
    va_start(ap, fmt);
    vsnprintf(*buf + *len, *cap - *len, fmt, ap);
    va_end(ap);
    *len += (size_t)need;
    return 1;
}
