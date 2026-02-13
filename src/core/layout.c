#include "core/layout.h"

#include <ctype.h>
#include <ncurses.h>
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

static int str_eq_ci(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int parse_profile(const char *val, LayoutProfile *out) {
    if (!val || !out) return 0;
    if (strcmp(val, "classic") == 0) {
        *out = LAYOUT_PROFILE_CLASSIC;
        return 1;
    }
    if (strcmp(val, "quad") == 0) {
        *out = LAYOUT_PROFILE_QUAD;
        return 1;
    }
    if (strcmp(val, "focus_editor") == 0) {
        *out = LAYOUT_PROFILE_FOCUS_EDITOR;
        return 1;
    }
    return 0;
}

static int parse_slot(const char *val, LayoutSlot *out) {
    if (!val || !out) return 0;
    if (strcmp(val, "tl") == 0) {
        *out = LAYOUT_SLOT_TL;
        return 1;
    }
    if (strcmp(val, "tr") == 0) {
        *out = LAYOUT_SLOT_TR;
        return 1;
    }
    if (strcmp(val, "bl") == 0) {
        *out = LAYOUT_SLOT_BL;
        return 1;
    }
    if (strcmp(val, "br") == 0) {
        *out = LAYOUT_SLOT_BR;
        return 1;
    }
    return 0;
}

static int parse_bool(const char *val, int *out) {
    if (!val || !out) return 0;
    if (str_eq_ci(val, "1") || str_eq_ci(val, "true") || str_eq_ci(val, "yes") || str_eq_ci(val, "on")) {
        *out = 1;
        return 1;
    }
    if (str_eq_ci(val, "0") || str_eq_ci(val, "false") || str_eq_ci(val, "no") || str_eq_ci(val, "off")) {
        *out = 0;
        return 1;
    }
    return 0;
}

static int parse_int_in_range(const char *val, int lo, int hi, int *out) {
    if (!val || !out) return 0;

    char *end = NULL;
    long n = strtol(val, &end, 10);
    if (end == val || *end != '\0') return 0;
    if (n < lo || n > hi) return 0;

    *out = (int)n;
    return 1;
}

static int parse_color_name(const char *val, int *out) {
    if (!val || !out) return 0;

    if (str_eq_ci(val, "default") || str_eq_ci(val, "-1")) {
        *out = -1;
        return 1;
    }
    if (str_eq_ci(val, "black")) {
        *out = COLOR_BLACK;
        return 1;
    }
    if (str_eq_ci(val, "red")) {
        *out = COLOR_RED;
        return 1;
    }
    if (str_eq_ci(val, "green")) {
        *out = COLOR_GREEN;
        return 1;
    }
    if (str_eq_ci(val, "yellow")) {
        *out = COLOR_YELLOW;
        return 1;
    }
    if (str_eq_ci(val, "blue")) {
        *out = COLOR_BLUE;
        return 1;
    }
    if (str_eq_ci(val, "magenta")) {
        *out = COLOR_MAGENTA;
        return 1;
    }
    if (str_eq_ci(val, "cyan")) {
        *out = COLOR_CYAN;
        return 1;
    }
    if (str_eq_ci(val, "white")) {
        *out = COLOR_WHITE;
        return 1;
    }

    return 0;
}

static const char *slot_name(LayoutSlot slot) {
    switch (slot) {
        case LAYOUT_SLOT_TL:
            return "tl";
        case LAYOUT_SLOT_TR:
            return "tr";
        case LAYOUT_SLOT_BL:
            return "bl";
        case LAYOUT_SLOT_BR:
            return "br";
        default:
            return "tl";
    }
}

static const char *color_name(int c) {
    switch (c) {
        case -1:
            return "default";
        case COLOR_BLACK:
            return "black";
        case COLOR_RED:
            return "red";
        case COLOR_GREEN:
            return "green";
        case COLOR_YELLOW:
            return "yellow";
        case COLOR_BLUE:
            return "blue";
        case COLOR_MAGENTA:
            return "magenta";
        case COLOR_CYAN:
            return "cyan";
        case COLOR_WHITE:
            return "white";
        default:
            return "default";
    }
}

static void set_defaults(
    LayoutProfile *out_profile,
    LayoutSlot *out_history_slot,
    LayoutSlot *out_editor_slot,
    LayoutSlot *out_response_slot,
    LayoutSizing *out_sizing,
    LayoutTheme *out_theme,
    int *out_show_footer_hint
) {
    if (out_profile) *out_profile = LAYOUT_PROFILE_CLASSIC;
    if (out_history_slot) *out_history_slot = LAYOUT_SLOT_TL;
    if (out_editor_slot) *out_editor_slot = LAYOUT_SLOT_TR;
    if (out_response_slot) *out_response_slot = LAYOUT_SLOT_BR;

    if (out_sizing) {
        out_sizing->classic_history_width_pct = 33;
        out_sizing->classic_editor_height_pct = 50;
        out_sizing->focus_editor_height_pct = 66;
        out_sizing->quad_split_x_pct = 50;
        out_sizing->quad_split_y_pct = 50;
    }

    if (out_theme) {
        out_theme->use_colors = 1;

        out_theme->focus_fg = COLOR_CYAN;
        out_theme->focus_bg = -1;

        out_theme->tab_active_fg = COLOR_BLACK;
        out_theme->tab_active_bg = COLOR_CYAN;

        out_theme->gutter_fg = COLOR_BLUE;
        out_theme->gutter_bg = -1;

        out_theme->warn_fg = COLOR_YELLOW;
        out_theme->warn_bg = -1;

        out_theme->error_fg = COLOR_RED;
        out_theme->error_bg = -1;
    }

    if (out_show_footer_hint) {
        *out_show_footer_hint = 1;
    }
}

static void set_default_theme(LayoutTheme *theme) {
    set_defaults(NULL, NULL, NULL, NULL, NULL, theme, NULL);
}

static void theme_set_mono(LayoutTheme *theme) {
    if (!theme) return;
    theme->use_colors = 0;
}

static void theme_set_vivid(LayoutTheme *theme) {
    if (!theme) return;
    theme->use_colors = 1;

    theme->focus_fg = COLOR_YELLOW;
    theme->focus_bg = COLOR_BLUE;

    theme->tab_active_fg = COLOR_BLACK;
    theme->tab_active_bg = COLOR_GREEN;

    theme->gutter_fg = COLOR_CYAN;
    theme->gutter_bg = -1;

    theme->warn_fg = COLOR_BLACK;
    theme->warn_bg = COLOR_YELLOW;

    theme->error_fg = COLOR_WHITE;
    theme->error_bg = COLOR_RED;
}

static int slots_are_unique(LayoutSlot a, LayoutSlot b, LayoutSlot c) {
    return (a != b) && (a != c) && (b != c);
}

int layout_load_config(
    const char *path,
    LayoutProfile *out_profile,
    LayoutSlot *out_history_slot,
    LayoutSlot *out_editor_slot,
    LayoutSlot *out_response_slot,
    LayoutSizing *out_sizing,
    LayoutTheme *out_theme,
    int *out_show_footer_hint,
    char *out_theme_preset,
    size_t out_theme_preset_size
) {
    if (!out_profile || !out_history_slot || !out_editor_slot || !out_response_slot || !out_sizing || !out_theme) {
        return 1;
    }

    if (out_theme_preset && out_theme_preset_size > 0) {
        out_theme_preset[0] = '\0';
    }

    set_defaults(
        out_profile,
        out_history_slot,
        out_editor_slot,
        out_response_slot,
        out_sizing,
        out_theme,
        out_show_footer_hint
    );

    if (!path) return 0;

    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char line[256];
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

        if (strcmp(key, "profile") == 0) {
            LayoutProfile p;
            if (parse_profile(val, &p)) *out_profile = p;
            continue;
        }

        if (strcmp(key, "theme_preset") == 0) {
            if (out_theme_preset && out_theme_preset_size > 0) {
                strncpy(out_theme_preset, val, out_theme_preset_size - 1);
                out_theme_preset[out_theme_preset_size - 1] = '\0';
            }
            continue;
        }

        if (strcmp(key, "show_footer_hint") == 0) {
            if (out_show_footer_hint) {
                (void)parse_bool(val, out_show_footer_hint);
            }
            continue;
        }

        if (strcmp(key, "quad_history_slot") == 0) {
            LayoutSlot s;
            if (parse_slot(val, &s)) *out_history_slot = s;
            continue;
        }
        if (strcmp(key, "quad_editor_slot") == 0) {
            LayoutSlot s;
            if (parse_slot(val, &s)) *out_editor_slot = s;
            continue;
        }
        if (strcmp(key, "quad_response_slot") == 0) {
            LayoutSlot s;
            if (parse_slot(val, &s)) *out_response_slot = s;
            continue;
        }

        if (strcmp(key, "classic_history_width_pct") == 0) {
            (void)parse_int_in_range(val, 20, 70, &out_sizing->classic_history_width_pct);
            continue;
        }
        if (strcmp(key, "classic_editor_height_pct") == 0) {
            (void)parse_int_in_range(val, 25, 75, &out_sizing->classic_editor_height_pct);
            continue;
        }
        if (strcmp(key, "focus_editor_height_pct") == 0) {
            (void)parse_int_in_range(val, 40, 85, &out_sizing->focus_editor_height_pct);
            continue;
        }
        if (strcmp(key, "quad_split_x_pct") == 0) {
            (void)parse_int_in_range(val, 30, 70, &out_sizing->quad_split_x_pct);
            continue;
        }
        if (strcmp(key, "quad_split_y_pct") == 0) {
            (void)parse_int_in_range(val, 30, 70, &out_sizing->quad_split_y_pct);
            continue;
        }

        if (strcmp(key, "use_colors") == 0) {
            (void)parse_bool(val, &out_theme->use_colors);
            continue;
        }

        if (strcmp(key, "focus_fg") == 0) {
            (void)parse_color_name(val, &out_theme->focus_fg);
            continue;
        }
        if (strcmp(key, "focus_bg") == 0) {
            (void)parse_color_name(val, &out_theme->focus_bg);
            continue;
        }
        if (strcmp(key, "tab_active_fg") == 0) {
            (void)parse_color_name(val, &out_theme->tab_active_fg);
            continue;
        }
        if (strcmp(key, "tab_active_bg") == 0) {
            (void)parse_color_name(val, &out_theme->tab_active_bg);
            continue;
        }
        if (strcmp(key, "gutter_fg") == 0) {
            (void)parse_color_name(val, &out_theme->gutter_fg);
            continue;
        }
        if (strcmp(key, "gutter_bg") == 0) {
            (void)parse_color_name(val, &out_theme->gutter_bg);
            continue;
        }
        if (strcmp(key, "warn_fg") == 0) {
            (void)parse_color_name(val, &out_theme->warn_fg);
            continue;
        }
        if (strcmp(key, "warn_bg") == 0) {
            (void)parse_color_name(val, &out_theme->warn_bg);
            continue;
        }
        if (strcmp(key, "error_fg") == 0) {
            (void)parse_color_name(val, &out_theme->error_fg);
            continue;
        }
        if (strcmp(key, "error_bg") == 0) {
            (void)parse_color_name(val, &out_theme->error_bg);
            continue;
        }
    }
    fclose(f);

    if (*out_profile == LAYOUT_PROFILE_QUAD &&
        !slots_are_unique(*out_history_slot, *out_editor_slot, *out_response_slot)) {
        *out_history_slot = LAYOUT_SLOT_TL;
        *out_editor_slot = LAYOUT_SLOT_TR;
        *out_response_slot = LAYOUT_SLOT_BR;
    }

    return 0;
}

int layout_theme_apply_preset(LayoutTheme *theme, const char *preset_name) {
    if (!theme || !preset_name) return 1;

    if (str_eq_ci(preset_name, "mono")) {
        set_default_theme(theme);
        theme_set_mono(theme);
        return 0;
    }

    if (str_eq_ci(preset_name, "vivid")) {
        set_default_theme(theme);
        theme_set_vivid(theme);
        return 0;
    }

    return 1;
}

void layout_theme_catalog_init(ThemeCatalog *c) {
    if (!c) return;
    c->items = NULL;
    c->count = 0;
    c->capacity = 0;
}

void layout_theme_catalog_free(ThemeCatalog *c) {
    if (!c) return;

    for (int i = 0; i < c->count; i++) {
        free(c->items[i].name);
    }
    free(c->items);

    c->items = NULL;
    c->count = 0;
    c->capacity = 0;
}

static int theme_catalog_ensure(ThemeCatalog *c, int need) {
    if (!c) return 1;
    if (c->capacity >= need) return 0;

    int new_cap = c->capacity ? c->capacity : 4;
    while (new_cap < need) new_cap *= 2;

    ThemePreset *n = realloc(c->items, (size_t)new_cap * sizeof(*n));
    if (!n) return 1;

    c->items = n;
    c->capacity = new_cap;
    return 0;
}

static int theme_catalog_add(ThemeCatalog *c, const char *name, const LayoutTheme *theme) {
    if (!c || !name || !theme || !name[0]) return 1;

    if (theme_catalog_ensure(c, c->count + 1) != 0) return 1;

    c->items[c->count].name = strdup(name);
    if (!c->items[c->count].name) return 1;

    c->items[c->count].theme = *theme;
    c->count++;
    return 0;
}

static int theme_catalog_add_builtin_presets(ThemeCatalog *c) {
    LayoutTheme t;

    set_default_theme(&t);
    theme_set_mono(&t);
    if (theme_catalog_add(c, "mono", &t) != 0) return 1;

    set_default_theme(&t);
    theme_set_vivid(&t);
    if (theme_catalog_add(c, "vivid", &t) != 0) return 1;

    return 0;
}

int layout_theme_catalog_load(const char *path, ThemeCatalog *c) {
    if (!c) return 1;

    layout_theme_catalog_free(c);
    layout_theme_catalog_init(c);

    FILE *f = fopen(path, "r");
    if (!f) {
        return theme_catalog_add_builtin_presets(c);
    }

    int rc = 0;
    int have_section = 0;
    char section_name[64] = {0};
    LayoutTheme current_theme;
    set_default_theme(&current_theme);

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        trim(line);
        if (line[0] == '\0') continue;

        size_t n = strlen(line);
        if (line[0] == '[' && n > 2 && line[n - 1] == ']') {
            if (have_section) {
                if (theme_catalog_add(c, section_name, &current_theme) != 0) {
                    rc = 1;
                    break;
                }
            }

            line[n - 1] = '\0';
            char *name = line + 1;
            trim(name);
            if (!name[0]) {
                have_section = 0;
                continue;
            }

            size_t sec_len = strlen(name);
            if (sec_len >= sizeof(section_name)) sec_len = sizeof(section_name) - 1;
            memcpy(section_name, name, sec_len);
            section_name[sec_len] = '\0';
            set_default_theme(&current_theme);
            have_section = 1;
            continue;
        }

        if (!have_section) continue;

        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';

        char *key = line;
        char *val = eq + 1;
        trim(key);
        trim(val);

        if (strcmp(key, "use_colors") == 0) {
            (void)parse_bool(val, &current_theme.use_colors);
            continue;
        }
        if (strcmp(key, "focus_fg") == 0) {
            (void)parse_color_name(val, &current_theme.focus_fg);
            continue;
        }
        if (strcmp(key, "focus_bg") == 0) {
            (void)parse_color_name(val, &current_theme.focus_bg);
            continue;
        }
        if (strcmp(key, "tab_active_fg") == 0) {
            (void)parse_color_name(val, &current_theme.tab_active_fg);
            continue;
        }
        if (strcmp(key, "tab_active_bg") == 0) {
            (void)parse_color_name(val, &current_theme.tab_active_bg);
            continue;
        }
        if (strcmp(key, "gutter_fg") == 0) {
            (void)parse_color_name(val, &current_theme.gutter_fg);
            continue;
        }
        if (strcmp(key, "gutter_bg") == 0) {
            (void)parse_color_name(val, &current_theme.gutter_bg);
            continue;
        }
        if (strcmp(key, "warn_fg") == 0) {
            (void)parse_color_name(val, &current_theme.warn_fg);
            continue;
        }
        if (strcmp(key, "warn_bg") == 0) {
            (void)parse_color_name(val, &current_theme.warn_bg);
            continue;
        }
        if (strcmp(key, "error_fg") == 0) {
            (void)parse_color_name(val, &current_theme.error_fg);
            continue;
        }
        if (strcmp(key, "error_bg") == 0) {
            (void)parse_color_name(val, &current_theme.error_bg);
            continue;
        }
    }

    if (rc == 0 && have_section) {
        if (theme_catalog_add(c, section_name, &current_theme) != 0) {
            rc = 1;
        }
    }

    fclose(f);

    if (rc != 0) {
        layout_theme_catalog_free(c);
        layout_theme_catalog_init(c);
        return theme_catalog_add_builtin_presets(c);
    }

    if (c->count == 0) {
        return theme_catalog_add_builtin_presets(c);
    }

    return 0;
}

int layout_theme_catalog_apply(const ThemeCatalog *c, const char *name, LayoutTheme *out) {
    if (!c || !name || !out) return 1;

    for (int i = 0; i < c->count; i++) {
        if (str_eq_ci(c->items[i].name, name)) {
            *out = c->items[i].theme;
            return 0;
        }
    }
    return 1;
}

char *layout_theme_catalog_list_names(const ThemeCatalog *c) {
    if (!c || c->count <= 0) {
        return strdup("No themes available");
    }

    size_t total = strlen("Available themes:\n") + 1;
    for (int i = 0; i < c->count; i++) {
        total += 4 + strlen(c->items[i].name) + 1;
    }

    char *out = malloc(total);
    if (!out) return NULL;

    size_t pos = 0;
    pos += (size_t)snprintf(out + pos, total - pos, "Available themes:\n");
    for (int i = 0; i < c->count; i++) {
        pos += (size_t)snprintf(out + pos, total - pos, "- %s\n", c->items[i].name);
    }

    return out;
}

int layout_save_config(
    const char *path,
    LayoutProfile profile,
    LayoutSlot history_slot,
    LayoutSlot editor_slot,
    LayoutSlot response_slot,
    const LayoutSizing *sizing,
    const LayoutTheme *theme,
    int show_footer_hint,
    const char *theme_preset
) {
    if (!path || !sizing || !theme) return 1;

    FILE *f = fopen(path, "w");
    if (!f) return 1;

    if (fprintf(
            f,
            "# Layout profile:\n"
            "#   classic      -> History left, Editor top-right, Response bottom-right\n"
            "#   quad         -> 2x2 grid with configurable panel slots\n"
            "#   focus_editor -> Large editor on top, History/Response split at bottom\n"
            "profile = %s\n"
            "theme_preset = %s\n"
            "# Footer quick hint in lower-right corner\n"
            "# true/false, yes/no, on/off, 1/0\n"
            "show_footer_hint = %s\n"
            "\n"
            "# Used only when profile = quad\n"
            "# tl tr bl br\n"
            "quad_history_slot = %s\n"
            "quad_editor_slot = %s\n"
            "quad_response_slot = %s\n"
            "\n"
            "# Layout sizing (percentages)\n"
            "# classic_history_width_pct: 20..70\n"
            "# classic_editor_height_pct: 25..75\n"
            "# focus_editor_height_pct: 40..85\n"
            "# quad_split_x_pct / quad_split_y_pct: 30..70\n"
            "classic_history_width_pct = %d\n"
            "classic_editor_height_pct = %d\n"
            "focus_editor_height_pct = %d\n"
            "quad_split_x_pct = %d\n"
            "quad_split_y_pct = %d\n"
            "\n"
            "# Theme fallback (used when theme_preset is empty or unknown)\n"
            "# use_colors accepts: true/false, yes/no, on/off, 1/0\n"
            "use_colors = %s\n"
            "#\n"
            "# Colors accept: default, black, red, green, yellow, blue, magenta, cyan, white\n"
            "focus_fg = %s\n"
            "focus_bg = %s\n"
            "tab_active_fg = %s\n"
            "tab_active_bg = %s\n"
            "gutter_fg = %s\n"
            "gutter_bg = %s\n"
            "warn_fg = %s\n"
            "warn_bg = %s\n"
            "error_fg = %s\n"
            "error_bg = %s\n",
            layout_profile_name(profile),
            (theme_preset && theme_preset[0]) ? theme_preset : "",
            show_footer_hint ? "true" : "false",
            slot_name(history_slot),
            slot_name(editor_slot),
            slot_name(response_slot),
            sizing->classic_history_width_pct,
            sizing->classic_editor_height_pct,
            sizing->focus_editor_height_pct,
            sizing->quad_split_x_pct,
            sizing->quad_split_y_pct,
            theme->use_colors ? "true" : "false",
            color_name(theme->focus_fg),
            color_name(theme->focus_bg),
            color_name(theme->tab_active_fg),
            color_name(theme->tab_active_bg),
            color_name(theme->gutter_fg),
            color_name(theme->gutter_bg),
            color_name(theme->warn_fg),
            color_name(theme->warn_bg),
            color_name(theme->error_fg),
            color_name(theme->error_bg)
        ) < 0) {
        fclose(f);
        return 1;
    }

    if (fclose(f) != 0) return 1;
    return 0;
}

const char *layout_profile_name(LayoutProfile profile) {
    switch (profile) {
        case LAYOUT_PROFILE_CLASSIC:
            return "classic";
        case LAYOUT_PROFILE_QUAD:
            return "quad";
        case LAYOUT_PROFILE_FOCUS_EDITOR:
            return "focus_editor";
        default:
            return "classic";
    }
}
