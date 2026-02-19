#include "core/auth.h"
#include "core/utils.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
        char *p = str_trim_left(tmp);
        str_trim_right(p);

        char *colon = strchr(p, ':');
        if (colon) {
            *colon = '\0';
            str_trim_right(p);
            if (!replaced && str_eq_ci(p, key)) {
                if (!str_appendf(&out, &len, &cap, "%s: %s\n", key, value)) {
                    free(tmp);
                    free(all);
                    free(out);
                    return 1;
                }
                replaced = 1;
            } else {
                if (!str_appendf(&out, &len, &cap, "%s\n", line)) {
                    free(tmp);
                    free(all);
                    free(out);
                    return 1;
                }
            }
        } else {
            if (!str_appendf(&out, &len, &cap, "%s\n", line)) {
                free(tmp);
                free(all);
                free(out);
                return 1;
            }
        }

        free(tmp);
    }

    if (!replaced) {
        if (!str_appendf(&out, &len, &cap, "%s: %s\n", key, value)) {
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
