#include "core/history_storage.h"

#include "state.h"
#include "core/cjson_compat.h"
#include "core/textbuf.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void trim(char *s) {
    char *p = s;
    while (*p && isspace((unsigned char)*p)) p++;
    if (p != s) memmove(s, p, strlen(p) + 1);

    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1])) {
        s[n - 1] = '\0';
        n--;
    }
}

static int ensure_parent_dirs(const char *path) {
    char *tmp = strdup(path);
    if (!tmp) return 1;

    char *slash = strrchr(tmp, '/');
    if (!slash) {
        free(tmp);
        return 0;
    }
    *slash = '\0';

    if (tmp[0] == '\0') {
        free(tmp);
        return 0;
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
            free(tmp);
            return 1;
        }
        *p = '/';
    }

    if (mkdir(tmp, 0700) != 0 && errno != EEXIST) {
        free(tmp);
        return 1;
    }

    free(tmp);
    return 0;
}

static char *json_dup_string_or_null(const cJSON *obj, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!v || cJSON_IsNull(v)) return NULL;
    if (!cJSON_IsString(v) || !v->valuestring) return NULL;
    return strdup(v->valuestring);
}

static char *json_dup_string_or_empty(const cJSON *obj, const char *key) {
    char *s = json_dup_string_or_null(obj, key);
    if (s) return s;
    return strdup("");
}

static int json_get_int(const cJSON *obj, const char *key, int def) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!v || !cJSON_IsNumber(v)) return def;
    return v->valueint;
}

static long json_get_long(const cJSON *obj, const char *key, long def) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!v || !cJSON_IsNumber(v)) return def;
    return (long)v->valuedouble;
}

static double json_get_double(const cJSON *obj, const char *key, double def) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive((cJSON *)obj, key);
    if (!v || !cJSON_IsNumber(v)) return def;
    return v->valuedouble;
}

char *history_storage_default_path(void) {
    const char *home = getenv("HOME");
    if (!home || !*home) return strdup("./history.jsonl");

    const char *suffix = "/.config/tcurl/history.jsonl";
    size_t n = strlen(home) + strlen(suffix) + 1;
    char *out = malloc(n);
    if (!out) return NULL;

    snprintf(out, n, "%s%s", home, suffix);
    return out;
}

int history_storage_load(History *h, const char *path) {
    return history_storage_load_with_stats(h, path, NULL);
}

int history_storage_load_with_stats(History *h, const char *path, HistoryLoadStats *stats) {
    if (!h || !path) return 1;

    if (stats) {
        stats->loaded_ok = 0;
        stats->skipped_invalid = 0;
    }

    FILE *f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) return 0;
        return 1;
    }

    char line[65536];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (line[0] == '\0') continue;

        cJSON *root = cJSON_Parse(line);
        if (!root) {
            if (stats) stats->skipped_invalid++;
            continue;
        }

        int method = json_get_int(root, "method", 0);
        long status = json_get_long(root, "status", 0);
        double elapsed_ms = json_get_double(root, "elapsed_ms", 0.0);
        int is_json = json_get_int(root, "is_json", 0);

        char *url = json_dup_string_or_empty(root, "url");
        char *body = json_dup_string_or_empty(root, "body");
        char *headers = json_dup_string_or_empty(root, "headers");
        char *response_body = json_dup_string_or_null(root, "response_body");
        char *response_body_view = json_dup_string_or_null(root, "response_body_view");

        TextBuffer body_tb;
        TextBuffer headers_tb;
        tb_init(&body_tb);
        tb_init(&headers_tb);
        tb_set_from_string(&body_tb, body);
        tb_set_from_string(&headers_tb, headers);

        HttpResponse resp;
        memset(&resp, 0, sizeof(resp));
        resp.status = status;
        resp.elapsed_ms = elapsed_ms;
        resp.is_json = is_json;
        resp.body = response_body ? strdup(response_body) : NULL;
        resp.body_view = response_body_view ? strdup(response_body_view) : NULL;
        resp.error = NULL;

        history_push(h, method, url, &body_tb, &headers_tb, &resp);
        if (stats) stats->loaded_ok++;

        tb_free(&body_tb);
        tb_free(&headers_tb);

        free(url);
        free(body);
        free(headers);
        free(response_body);
        free(response_body_view);
        free(resp.body);
        free(resp.body_view);

        cJSON_Delete(root);
    }

    fclose(f);
    return 0;
}

static int append_history_item(FILE *f, const HistoryItem *it) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return 1;

    cJSON_AddNumberToObject(root, "method", it->method);
    cJSON_AddStringToObject(root, "url", it->url ? it->url : "");
    cJSON_AddStringToObject(root, "body", it->body ? it->body : "");
    cJSON_AddStringToObject(root, "headers", it->headers ? it->headers : "");
    cJSON_AddNumberToObject(root, "status", it->status);
    cJSON_AddNumberToObject(root, "elapsed_ms", it->elapsed_ms);
    cJSON_AddNumberToObject(root, "is_json", it->is_json);

    if (it->response_body) cJSON_AddStringToObject(root, "response_body", it->response_body);
    else cJSON_AddNullToObject(root, "response_body");

    if (it->response_body_view) cJSON_AddStringToObject(root, "response_body_view", it->response_body_view);
    else cJSON_AddNullToObject(root, "response_body_view");

    char *line = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!line) return 1;

    int rc = 0;
    if (fputs(line, f) < 0 || fputc('\n', f) == EOF) rc = 1;
    free(line);
    return rc;
}

int history_storage_append_last(const History *h, const char *path) {
    if (!h || !path) return 1;
    if (h->count <= 0) return 0;
    if (ensure_parent_dirs(path) != 0) return 1;

    FILE *f = fopen(path, "a");
    if (!f) return 1;
    int rc = append_history_item(f, &h->items[h->count - 1]);
    if (fclose(f) != 0) rc = 1;
    return rc;
}

int history_storage_save(const History *h, const char *path) {
    if (!h || !path) return 1;
    if (ensure_parent_dirs(path) != 0) return 1;

    size_t tlen = strlen(path) + 5;
    char *tmp_path = malloc(tlen);
    if (!tmp_path) return 1;
    snprintf(tmp_path, tlen, "%s.tmp", path);

    FILE *f = fopen(tmp_path, "w");
    if (!f) {
        free(tmp_path);
        return 1;
    }

    int rc = 0;
    for (int i = 0; i < h->count; i++) {
        const HistoryItem *it = &h->items[i];
        if (append_history_item(f, it) != 0) {
            rc = 1;
            break;
        }
    }

    if (fclose(f) != 0) rc = 1;

    if (rc == 0) {
        if (rename(tmp_path, path) != 0) rc = 1;
    } else {
        unlink(tmp_path);
    }

    free(tmp_path);
    return rc;
}

int history_config_load_max_entries(const char *path, int fallback) {
    int out = fallback > 0 ? fallback : 500;
    if (!path) return out;

    FILE *f = fopen(path, "r");
    if (!f) return out;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        trim(line);
        if (line[0] == '\0') continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';

        char *key = line;
        char *val = eq + 1;
        trim(key);
        trim(val);

        if (strcmp(key, "max_entries") != 0) continue;

        char *end = NULL;
        long n = strtol(val, &end, 10);
        if (end && *end == '\0' && n > 0 && n <= 1000000) {
            out = (int)n;
        }
    }

    fclose(f);
    return out;
}
