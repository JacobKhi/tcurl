#include "core/auth.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int key_eq_ci(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static char *trim_left(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    return s;
}

static void trim_right(char *s) {
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
}

static int appendf(char **buf, size_t *len, size_t *cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int need = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (need < 0) return 1;

    size_t req = *len + (size_t)need + 1;
    if (*cap < req) {
        size_t new_cap = *cap ? *cap : 128;
        while (new_cap < req) new_cap *= 2;
        char *n = realloc(*buf, new_cap);
        if (!n) return 1;
        *buf = n;
        *cap = new_cap;
    }

    va_start(ap, fmt);
    vsnprintf(*buf + *len, *cap - *len, fmt, ap);
    va_end(ap);
    *len += (size_t)need;
    return 0;
}

static int set_header_value(TextBuffer *headers, const char *key, const char *value) {
    if (!headers || !key || !value) return 1;

    char *all = tb_to_string(headers);
    if (!all) return 1;

    char *out = NULL;
    size_t len = 0;
    size_t cap = 0;
    int replaced = 0;

    char *save = NULL;
    for (char *line = strtok_r(all, "\n", &save); line; line = strtok_r(NULL, "\n", &save)) {
        char *tmp = strdup(line);
        if (!tmp) {
            free(all);
            free(out);
            return 1;
        }
        char *p = trim_left(tmp);
        trim_right(p);

        char *colon = strchr(p, ':');
        if (colon) {
            *colon = '\0';
            trim_right(p);
            if (!replaced && key_eq_ci(p, key)) {
                if (appendf(&out, &len, &cap, "%s: %s\n", key, value) != 0) {
                    free(tmp);
                    free(all);
                    free(out);
                    return 1;
                }
                replaced = 1;
            } else {
                if (appendf(&out, &len, &cap, "%s\n", line) != 0) {
                    free(tmp);
                    free(all);
                    free(out);
                    return 1;
                }
            }
        } else {
            if (appendf(&out, &len, &cap, "%s\n", line) != 0) {
                free(tmp);
                free(all);
                free(out);
                return 1;
            }
        }

        free(tmp);
    }

    if (!replaced) {
        if (appendf(&out, &len, &cap, "%s: %s\n", key, value) != 0) {
            free(all);
            free(out);
            return 1;
        }
    }

    if (!out) {
        out = strdup("");
        if (!out) {
            free(all);
            return 1;
        }
    }

    if (len > 0 && out[len - 1] == '\n') {
        out[len - 1] = '\0';
    }

    tb_set_from_string(headers, out);

    free(all);
    free(out);
    return 0;
}

static char *base64_encode(const unsigned char *data, size_t len) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t out_len = ((len + 2) / 3) * 4;
    char *out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t p = 0;
    for (size_t i = 0; i < len; i += 3) {
        unsigned v = data[i] << 16;
        if (i + 1 < len) v |= data[i + 1] << 8;
        if (i + 2 < len) v |= data[i + 2];

        out[p++] = tbl[(v >> 18) & 0x3F];
        out[p++] = tbl[(v >> 12) & 0x3F];
        out[p++] = (i + 1 < len) ? tbl[(v >> 6) & 0x3F] : '=';
        out[p++] = (i + 2 < len) ? tbl[v & 0x3F] : '=';
    }
    out[p] = '\0';
    return out;
}

int auth_apply_bearer(TextBuffer *headers, const char *token) {
    if (!token || !token[0]) return 1;
    char value[2048];
    snprintf(value, sizeof(value), "Bearer %s", token);
    return set_header_value(headers, "Authorization", value);
}

int auth_apply_basic(TextBuffer *headers, const char *user, const char *pass) {
    if (!user || !pass) return 1;

    size_t n = strlen(user) + strlen(pass) + 2;
    char *plain = malloc(n);
    if (!plain) return 1;
    snprintf(plain, n, "%s:%s", user, pass);

    char *b64 = base64_encode((const unsigned char *)plain, strlen(plain));
    free(plain);
    if (!b64) return 1;

    char value[4096];
    snprintf(value, sizeof(value), "Basic %s", b64);
    free(b64);
    return set_header_value(headers, "Authorization", value);
}
