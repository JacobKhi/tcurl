#include "core/paths.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static char *path_strdup(const char *s) {
    if (!s) return NULL;
    return strdup(s);
}

static char *path_join(const char *a, const char *b) {
    if (!a || !b) return NULL;

    size_t a_len = strlen(a);
    size_t b_len = strlen(b);
    int need_sep = (a_len > 0 && a[a_len - 1] != '/');

    size_t out_len = a_len + (need_sep ? 1 : 0) + b_len + 1;
    char *out = malloc(out_len);
    if (!out) return NULL;

    strcpy(out, a);
    if (need_sep) strcat(out, "/");
    strcat(out, b);
    return out;
}

static char *path_to_absolute(const char *path) {
    if (!path || !path[0]) return NULL;
    if (path[0] == '/') return path_strdup(path);

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return NULL;
    return path_join(cwd, path);
}

static char *path_dirname_dup(const char *path) {
    if (!path || !path[0]) return NULL;

    const char *slash = strrchr(path, '/');
    if (!slash) return path_strdup(".");
    if (slash == path) return path_strdup("/");

    size_t len = (size_t)(slash - path);
    char *out = malloc(len + 1);
    if (!out) return NULL;
    memcpy(out, path, len);
    out[len] = '\0';
    return out;
}

static char *resolve_executable_dir(void) {
    char link_buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", link_buf, sizeof(link_buf) - 1);
    if (n > 0) {
        link_buf[n] = '\0';
        return path_dirname_dup(link_buf);
    }

    char cwd[PATH_MAX];
    if (!getcwd(cwd, sizeof(cwd))) return NULL;
    return path_strdup(cwd);
}

static int path_exists(const char *path) {
    struct stat st;
    return (path && stat(path, &st) == 0);
}

static int mkdir_p(const char *dir_path) {
    if (!dir_path || !dir_path[0]) return 1;

    char *tmp = strdup(dir_path);
    if (!tmp) return 1;

    size_t len = strlen(tmp);
    if (len > 1 && tmp[len - 1] == '/') {
        tmp[len - 1] = '\0';
    }

    for (char *p = tmp + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        if (mkdir(tmp, 0700) != 0 && access(tmp, F_OK) != 0) {
            free(tmp);
            return 1;
        }
        *p = '/';
    }

    if (mkdir(tmp, 0700) != 0 && access(tmp, F_OK) != 0) {
        free(tmp);
        return 1;
    }

    free(tmp);
    return 0;
}

static char *pick_read_path(const char *user_path, const char *fallback_path) {
    if (path_exists(user_path)) return path_strdup(user_path);
    if (path_exists(fallback_path)) return path_strdup(fallback_path);
    return path_strdup(user_path);
}

void paths_init(AppPaths *p) {
    if (!p) return;
    memset(p, 0, sizeof(*p));
}

void paths_free(AppPaths *p) {
    if (!p) return;

    free(p->config_dir);
    free(p->fallback_config_dir);
    free(p->keymap_conf);
    free(p->layout_conf);
    free(p->layout_conf_load);
    free(p->themes_conf);
    free(p->envs_json);
    free(p->headers_txt);
    free(p->history_conf);

    memset(p, 0, sizeof(*p));
}

int paths_resolve_config_dir(AppPaths *p) {
    if (!p) return 1;

    char *config_dir = NULL;

    const char *override_dir = getenv("TCURL_CONFIG_DIR");
    if (override_dir && override_dir[0]) {
        config_dir = path_to_absolute(override_dir);
    } else {
        const char *xdg_home = getenv("XDG_CONFIG_HOME");
        if (xdg_home && xdg_home[0]) {
            char *base = path_to_absolute(xdg_home);
            if (!base) return 1;
            config_dir = path_join(base, "tcurl");
            free(base);
        } else {
            const char *home = getenv("HOME");
            if (home && home[0]) {
                config_dir = path_join(home, ".config/tcurl");
            } else {
                char cwd[PATH_MAX];
                if (!getcwd(cwd, sizeof(cwd))) return 1;
                config_dir = path_join(cwd, ".config/tcurl");
            }
        }
    }

    if (!config_dir) return 1;

    char *exe_dir = resolve_executable_dir();
    if (!exe_dir) {
        free(config_dir);
        return 1;
    }

    char *fallback = path_join(exe_dir, "config");
    free(exe_dir);
    if (!fallback) {
        free(config_dir);
        return 1;
    }

    if (mkdir_p(config_dir) != 0) {
        free(config_dir);
        free(fallback);
        return 1;
    }

    p->config_dir = config_dir;
    p->fallback_config_dir = fallback;
    return 0;
}

int paths_build_file_paths(AppPaths *p) {
    if (!p || !p->config_dir || !p->fallback_config_dir) return 1;

    char *user_keymap = path_join(p->config_dir, "keymap.conf");
    char *user_layout = path_join(p->config_dir, "layout.conf");
    char *user_themes = path_join(p->config_dir, "themes.conf");
    char *user_envs = path_join(p->config_dir, "envs.json");
    char *user_headers = path_join(p->config_dir, "headers.txt");
    char *user_history = path_join(p->config_dir, "history.conf");

    char *fallback_keymap = path_join(p->fallback_config_dir, "keymap.conf");
    char *fallback_layout = path_join(p->fallback_config_dir, "layout.conf");
    char *fallback_themes = path_join(p->fallback_config_dir, "themes.conf");
    char *fallback_envs = path_join(p->fallback_config_dir, "envs.json");
    char *fallback_headers = path_join(p->fallback_config_dir, "headers.txt");
    char *fallback_history = path_join(p->fallback_config_dir, "history.conf");

    if (!user_keymap || !user_layout || !user_themes || !user_envs ||
        !user_headers || !user_history || !fallback_keymap || !fallback_layout ||
        !fallback_themes || !fallback_envs || !fallback_headers || !fallback_history) {
        free(user_keymap);
        free(user_layout);
        free(user_themes);
        free(user_envs);
        free(user_headers);
        free(user_history);
        free(fallback_keymap);
        free(fallback_layout);
        free(fallback_themes);
        free(fallback_envs);
        free(fallback_headers);
        free(fallback_history);
        return 1;
    }

    p->keymap_conf = pick_read_path(user_keymap, fallback_keymap);
    p->layout_conf = user_layout;
    p->layout_conf_load = pick_read_path(user_layout, fallback_layout);
    p->themes_conf = pick_read_path(user_themes, fallback_themes);
    p->envs_json = pick_read_path(user_envs, fallback_envs);
    p->headers_txt = pick_read_path(user_headers, fallback_headers);
    p->history_conf = pick_read_path(user_history, fallback_history);

    free(user_keymap);
    free(user_themes);
    free(user_envs);
    free(user_headers);
    free(user_history);
    free(fallback_keymap);
    free(fallback_layout);
    free(fallback_themes);
    free(fallback_envs);
    free(fallback_headers);
    free(fallback_history);

    if (!p->keymap_conf || !p->layout_conf || !p->layout_conf_load || !p->themes_conf ||
        !p->envs_json || !p->headers_txt || !p->history_conf) {
        return 1;
    }

    return 0;
}
