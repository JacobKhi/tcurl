#pragma once

#include <stddef.h>

typedef enum {
    LAYOUT_PROFILE_CLASSIC = 0,
    LAYOUT_PROFILE_QUAD = 1,
    LAYOUT_PROFILE_FOCUS_EDITOR = 2
} LayoutProfile;

typedef enum {
    LAYOUT_SLOT_TL = 0,
    LAYOUT_SLOT_TR = 1,
    LAYOUT_SLOT_BL = 2,
    LAYOUT_SLOT_BR = 3
} LayoutSlot;

typedef struct {
    int classic_history_width_pct;
    int classic_editor_height_pct;
    int focus_editor_height_pct;
    int quad_split_x_pct;
    int quad_split_y_pct;
} LayoutSizing;

typedef struct {
    int use_colors;

    int focus_fg;
    int focus_bg;

    int tab_active_fg;
    int tab_active_bg;

    int gutter_fg;
    int gutter_bg;

    int warn_fg;
    int warn_bg;

    int error_fg;
    int error_bg;
} LayoutTheme;

typedef struct {
    char *name;
    LayoutTheme theme;
} ThemePreset;

typedef struct {
    ThemePreset *items;
    int count;
    int capacity;
} ThemeCatalog;

int layout_load_config(
    const char *path,
    LayoutProfile *out_profile,
    LayoutSlot *out_history_slot,
    LayoutSlot *out_editor_slot,
    LayoutSlot *out_response_slot,
    LayoutSizing *out_sizing,
    LayoutTheme *out_theme,
    char *out_theme_preset,
    size_t out_theme_preset_size
);

const char *layout_profile_name(LayoutProfile profile);

int layout_theme_apply_preset(LayoutTheme *theme, const char *preset_name);

void layout_theme_catalog_init(ThemeCatalog *c);
void layout_theme_catalog_free(ThemeCatalog *c);
int layout_theme_catalog_load(const char *path, ThemeCatalog *c);
int layout_theme_catalog_apply(const ThemeCatalog *c, const char *name, LayoutTheme *out);
char *layout_theme_catalog_list_names(const ThemeCatalog *c);

int layout_save_config(
    const char *path,
    LayoutProfile profile,
    LayoutSlot history_slot,
    LayoutSlot editor_slot,
    LayoutSlot response_slot,
    const LayoutSizing *sizing,
    const LayoutTheme *theme,
    const char *theme_preset
);
