#ifndef COMMAND_HANDLERS_H
#define COMMAND_HANDLERS_H

#include "state.h"

/**
 * Apply a theme preset to the application state.
 * 
 * @param s Application state
 * @param preset Theme preset name
 * @param save If 1, save to layout.conf; if 0, apply for session only
 */
void cmd_apply_theme(AppState *s, const char *preset, int save);

/**
 * List available theme presets.
 * 
 * @param s Application state
 */
void cmd_list_themes(AppState *s);

/**
 * Clear the request history.
 * 
 * @param s Application state
 */
void cmd_clear_history(AppState *s);

/**
 * Export current request in specified format.
 * 
 * @param s Application state
 * @param format Export format ("curl" or "json")
 */
void cmd_export_request(AppState *s, const char *format);

/**
 * Apply authentication to headers.
 * 
 * @param s Application state
 * @param kind Authentication kind ("bearer" or "basic")
 * @param arg Authentication argument (token for bearer, "user:pass" for basic)
 */
void cmd_auth(AppState *s, const char *kind, const char *arg);

/**
 * Find/search for a query string.
 * 
 * @param s Application state
 * @param query Search query
 */
void cmd_find(AppState *s, const char *query);

/**
 * Get or set configuration settings.
 * 
 * @param s Application state
 * @param key Setting key (NULL to display current settings)
 * @param value Setting value (NULL with valid key to show usage)
 */
void cmd_set(AppState *s, const char *key, const char *value);

/**
 * Change UI language.
 * 
 * @param s Application state
 * @param arg Language code or "list"
 */
void cmd_lang(AppState *s, const char *arg);

/**
 * Change UI layout.
 * 
 * @param s Application state
 * @param arg Layout name or "list"
 */
void cmd_layout(AppState *s, const char *arg);

#endif  // COMMAND_HANDLERS_H
