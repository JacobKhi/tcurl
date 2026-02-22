#ifndef HELP_BUILDER_H
#define HELP_BUILDER_H

#include "core/config/keymap.h"
#include "core/text/i18n.h"

/**
 * Build a comprehensive help text string showing all available commands and key bindings.
 * 
 * The help text includes:
 * - Basic commands (quit, help)
 * - Language commands
 * - Layout commands
 * - Theme management
 * - Export commands
 * - Authentication commands
 * - Search commands
 * - History commands
 * - Settings commands
 * - Navigation and key bindings for all modes
 * 
 * @param km Keymap containing key bindings
 * @param lang UI language for localized help text
 * @return Newly allocated help text string, or NULL on failure. Caller must free.
 */
char *help_build_text(const Keymap *km, UiLanguage lang);

#endif  // HELP_BUILDER_H
