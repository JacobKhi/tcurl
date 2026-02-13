#include "core/env.h"

#include <cjson/cJSON.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static char *read_all(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    long sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    char *buf = malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    buf[n] = '\0';
    return buf;
}

static void free_env_items(Environment *items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) {
        free(items[i].name);
        if (items[i].vars) {
            for (int j = 0; j < items[i].var_count; j++) {
                free(items[i].vars[j].key);
                free(items[i].vars[j].value);
            }
        }
        free(items[i].vars);
    }
    free(items);
}

void env_store_init(EnvStore *store) {
    if (!store) return;
    store->items = NULL;
    store->count = 0;
    store->active_index = -1;
}

void env_store_free(EnvStore *store) {
    if (!store) return;

    for (int i = 0; i < store->count; i++) {
        Environment *env = &store->items[i];
        free(env->name);

        for (int j = 0; j < env->var_count; j++) {
            free(env->vars[j].key);
            free(env->vars[j].value);
        }
        free(env->vars);
    }

    free(store->items);
    store->items = NULL;
    store->count = 0;
    store->active_index = -1;
}

static int is_env_object(cJSON *node) {
    return node && node->string && cJSON_IsObject(node);
}

int env_store_load_file(EnvStore *store, const char *path) {
    if (!store || !path) return 1;

    char *json = read_all(path);
    if (!json) {
        if (errno == ENOENT) return 0;
        return 1;
    }

    cJSON *root = cJSON_Parse(json);
    free(json);
    if (!root || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return 1;
    }

    int env_count = 0;
    for (cJSON *env = root->child; env; env = env->next) {
        if (is_env_object(env)) env_count++;
    }

    Environment *items = NULL;
    if (env_count > 0) {
        items = calloc((size_t)env_count, sizeof(*items));
        if (!items) {
            cJSON_Delete(root);
            return 1;
        }
    }

    int idx = 0;
    for (cJSON *env = root->child; env; env = env->next) {
        if (!is_env_object(env)) continue;

        Environment *out = &items[idx++];
        out->name = strdup(env->string);
        if (!out->name) {
            cJSON_Delete(root);
            free_env_items(items, idx);
            return 1;
        }

        int var_count = 0;
        for (cJSON *v = env->child; v; v = v->next) {
            if (v->string && cJSON_IsString(v) && v->valuestring) var_count++;
        }

        out->var_count = var_count;
        if (var_count > 0) {
            out->vars = calloc((size_t)var_count, sizeof(*out->vars));
            if (!out->vars) {
                cJSON_Delete(root);
                free_env_items(items, idx);
                return 1;
            }
        }

        int vidx = 0;
        for (cJSON *v = env->child; v; v = v->next) {
            if (!v->string || !cJSON_IsString(v) || !v->valuestring) continue;

            out->vars[vidx].key = strdup(v->string);
            out->vars[vidx].value = strdup(v->valuestring);
            if (!out->vars[vidx].key || !out->vars[vidx].value) {
                cJSON_Delete(root);
                free_env_items(items, idx);
                return 1;
            }
            vidx++;
        }
    }

    env_store_free(store);
    store->items = items;
    store->count = env_count;
    store->active_index = (env_count > 0) ? 0 : -1;

    for (int i = 0; i < store->count; i++) {
        if (strcmp(store->items[i].name, "dev") == 0) {
            store->active_index = i;
            break;
        }
    }

    cJSON_Delete(root);
    return 0;
}

const char *env_store_active_name(const EnvStore *store) {
    if (!store) return NULL;
    if (store->active_index < 0 || store->active_index >= store->count) return NULL;
    return store->items[store->active_index].name;
}

void env_store_cycle(EnvStore *store) {
    if (!store || store->count <= 0) return;
    store->active_index = (store->active_index + 1) % store->count;
}

const char *env_store_lookup(const EnvStore *store, const char *key) {
    if (!store || !key) return NULL;
    if (store->active_index < 0 || store->active_index >= store->count) return NULL;

    const Environment *env = &store->items[store->active_index];
    for (int i = 0; i < env->var_count; i++) {
        if (strcmp(env->vars[i].key, key) == 0) return env->vars[i].value;
    }
    return NULL;
}

static int is_valid_var_name(const char *s) {
    if (!s || !*s) return 0;
    if (!(isalpha((unsigned char)s[0]) || s[0] == '_')) return 0;
    for (int i = 1; s[i]; i++) {
        if (!(isalnum((unsigned char)s[i]) || s[i] == '_')) return 0;
    }
    return 1;
}

static int ensure_cap(char **buf, size_t *cap, size_t need) {
    if (*cap >= need) return 1;
    size_t new_cap = *cap ? *cap : 64;
    while (new_cap < need) new_cap *= 2;
    char *n = realloc(*buf, new_cap);
    if (!n) return 0;
    *buf = n;
    *cap = new_cap;
    return 1;
}

static int append_mem(char **buf, size_t *len, size_t *cap, const char *s, size_t n) {
    if (!ensure_cap(buf, cap, *len + n + 1)) return 0;
    memcpy(*buf + *len, s, n);
    *len += n;
    (*buf)[*len] = '\0';
    return 1;
}

char *env_expand_template(const EnvStore *store, const char *input, char **missing_name) {
    if (missing_name) *missing_name = NULL;
    if (!input) return strdup("");

    char *out = NULL;
    size_t out_len = 0;
    size_t out_cap = 0;

    if (!ensure_cap(&out, &out_cap, strlen(input) + 1)) return NULL;
    out[0] = '\0';

    size_t i = 0;
    while (input[i]) {
        if (input[i] == '{' && input[i + 1] == '{') {
            size_t j = i + 2;
            while (input[j] && !(input[j] == '}' && input[j + 1] == '}')) j++;

            if (input[j] == '}' && input[j + 1] == '}') {
                size_t n = j - (i + 2);
                char *name = malloc(n + 1);
                if (!name) {
                    free(out);
                    return NULL;
                }
                memcpy(name, input + i + 2, n);
                name[n] = '\0';

                if (is_valid_var_name(name)) {
                    const char *val = env_store_lookup(store, name);
                    if (!val) {
                        if (missing_name) *missing_name = name;
                        else free(name);
                        free(out);
                        return NULL;
                    }

                    if (!append_mem(&out, &out_len, &out_cap, val, strlen(val))) {
                        free(name);
                        free(out);
                        return NULL;
                    }
                    free(name);
                    i = j + 2;
                    continue;
                }

                free(name);
            }
        }

        if (!append_mem(&out, &out_len, &out_cap, input + i, 1)) {
            free(out);
            return NULL;
        }
        i++;
    }

    return out;
}

int header_suggestions_load(const char *path, char ***out_items, int *out_count) {
    if (!out_items || !out_count) return 1;

    *out_items = NULL;
    *out_count = 0;

    FILE *f = fopen(path, "r");
    if (!f) {
        if (errno == ENOENT) return 0;
        return 1;
    }

    char **items = NULL;
    int count = 0;
    int cap = 0;
    char line[512];

    while (fgets(line, sizeof(line), f)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        trim(line);
        if (line[0] == '\0') continue;

        if (count == cap) {
            int new_cap = cap ? cap * 2 : 8;
            char **n = realloc(items, (size_t)new_cap * sizeof(*n));
            if (!n) {
                fclose(f);
                header_suggestions_free(items, count);
                return 1;
            }
            items = n;
            cap = new_cap;
        }

        items[count] = strdup(line);
        if (!items[count]) {
            fclose(f);
            header_suggestions_free(items, count);
            return 1;
        }
        count++;
    }

    fclose(f);
    *out_items = items;
    *out_count = count;
    return 0;
}

void header_suggestions_free(char **items, int count) {
    if (!items) return;
    for (int i = 0; i < count; i++) free(items[i]);
    free(items);
}
