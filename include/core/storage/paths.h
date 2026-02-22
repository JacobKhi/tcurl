#pragma once

typedef struct {
    char *config_dir;
    char *fallback_config_dir;

    char *keymap_conf;
    char *layout_conf;
    char *layout_conf_load;
    char *themes_conf;
    char *envs_json;
    char *headers_txt;
    char *history_conf;
} AppPaths;

void paths_init(AppPaths *p);
void paths_free(AppPaths *p);

int paths_resolve_config_dir(AppPaths *p);
int paths_build_file_paths(AppPaths *p);
