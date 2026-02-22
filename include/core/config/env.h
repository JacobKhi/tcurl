#pragma once

typedef struct {
    char *key;
    char *value;
} EnvVar;

typedef struct {
    char *name;
    EnvVar *vars;
    int var_count;
} Environment;

typedef struct {
    Environment *items;
    int count;
    int active_index;
} EnvStore;

void env_store_init(EnvStore *store);
void env_store_free(EnvStore *store);

int env_store_load_file(EnvStore *store, const char *path);
const char *env_store_active_name(const EnvStore *store);
void env_store_cycle(EnvStore *store);
const char *env_store_lookup(const EnvStore *store, const char *key);

char *env_expand_template(const EnvStore *store, const char *input, char **missing_name);

int header_suggestions_load(const char *path, char ***out_items, int *out_count);
void header_suggestions_free(char **items, int count);
