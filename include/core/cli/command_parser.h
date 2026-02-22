#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include "state.h"
#include "core/config/keymap.h"

/**
 * Command handler function type.
 * Takes app state, keymap, and argument string.
 */
typedef void (*CommandHandler)(AppState *s, const Keymap *km, const char *args);

/**
 * Command registry entry.
 */
typedef struct {
    const char *name;        /* Command name (e.g., "quit", "help") */
    const char *alias;       /* Optional alias (e.g., "q" for "quit"), NULL if none */
    CommandHandler handler;  /* Handler function */
} CommandEntry;

/**
 * Parse and execute a command string.
 * 
 * @param s Application state
 * @param km Keymap for key binding resolution
 * @param cmd Command string from user input (with or without leading ':')
 */
void command_parse_and_execute(AppState *s, const Keymap *km, const char *cmd);

#endif  // COMMAND_PARSER_H
