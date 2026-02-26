#include "core/cli/command_parser.h"
#include "core/cli/command_handlers.h"
#include "core/cli/help_builder.h"
#include "core/text/i18n.h"
#include "state.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Forward declarations for local helpers */
static void clear_prompt(AppState *s);
static void finish_command(AppState *s);
static void response_set_error(AppState *s, const char *err);
static void response_set_text(AppState *s, const char *text);

/* Helper: Clear command prompt */
static void clear_prompt(AppState *s) {
    s->prompt.kind = PROMPT_NONE;
    s->prompt.input[0] = '\0';
    s->prompt.len = 0;
    s->prompt.cursor = 0;
}

/* Helper: Finish command execution (set mode, clear prompt) */
static void finish_command(AppState *s) {
    s->ui.mode = MODE_NORMAL;
    clear_prompt(s);
}

/* Helper: Set response error */
static void response_reset_content(AppState *s) {
    free(s->response.response.body);
    free(s->response.response.body_view);
    free(s->response.response.response_headers);
    free(s->response.response.error);

    s->response.response.body = NULL;
    s->response.response.body_view = NULL;
    s->response.response.response_headers = NULL;
    s->response.response.error = NULL;
    s->response.response.status = 0;
    s->response.response.elapsed_ms = 0.0;
    s->response.response.is_json = 0;
    s->response.scroll = 0;
}

static void response_set_text(AppState *s, const char *text) {
    response_reset_content(s);
    s->response.response.body = strdup(text ? text : "");
    s->response.response.body_view = strdup(text ? text : "");
    s->ui.focused_panel = PANEL_RESPONSE;
}

static void response_set_error(AppState *s, const char *err) {
    response_reset_content(s);
    s->response.response.error = strdup(err ? err : i18n_get(s->ui.language, I18N_UNKNOWN_ERROR));
    s->ui.focused_panel = PANEL_RESPONSE;
}

/* Command handlers */

static void handle_quit(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    (void)args;
    s->running = 0;
}

static void handle_help(AppState *s, const Keymap *km, const char *args) {
    (void)args;
    char *help = help_build_text(km, s->ui.language);
    if (!help) {
        response_set_error(s, i18n_get(s->ui.language, I18N_OOM_BUILD_HELP));
    } else {
        response_set_text(s, help);
        free(help);
    }
}

static void handle_theme(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    
    if (!args || !*args) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_THEME_NAME_SAVE_OR_LIST));
        return;
    }

    char buf[256];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *name = strtok(buf, " \t");
    char *flag = strtok(NULL, " \t");
    char *extra = strtok(NULL, " \t");

    if (!name || extra) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_THEME_NAME_SAVE_OR_LIST));
        return;
    }

    if (strcmp(name, "list") == 0) {
        if (flag) {
            response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_THEME_LIST));
            return;
        }
        cmd_list_themes(s);
        return;
    }

    int save = 0;
    if (flag) {
        if (strcmp(flag, "-s") == 0 || strcmp(flag, "--save") == 0) {
            save = 1;
        } else {
            response_set_error(s, i18n_get(s->ui.language, I18N_INVALID_THEME_FLAG));
            return;
        }
    }

    cmd_apply_theme(s, name, save);
}

static void handle_export(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    
    if (!args || !*args) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_EXPORT));
        return;
    }

    char buf[256];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *fmt = strtok(buf, " \t");
    char *extra = strtok(NULL, " \t");
    
    if (extra) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_EXPORT));
    } else {
        cmd_export_request(s, fmt);
    }
}

static void handle_auth(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    
    if (!args || !*args) {
        cmd_auth(s, NULL, NULL);
        return;
    }

    char buf[512];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *kind = buf;
    char *arg = NULL;
    
    /* Find space after kind */
    char *space = kind;
    while (*space && !isspace((unsigned char)*space)) space++;
    
    if (*space) {
        *space = '\0';
        arg = space + 1;
        while (*arg && isspace((unsigned char)*arg)) arg++;
        if (!*arg) arg = NULL;
    }
    
    cmd_auth(s, kind, arg);
}

static void handle_find(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    cmd_find(s, args ? args : "");
}

static void handle_set(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    
    if (!args || !*args) {
        cmd_set(s, NULL, NULL);
        return;
    }

    char buf[256];
    strncpy(buf, args, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *key = strtok(buf, " \t");
    char *val = strtok(NULL, " \t");
    char *extra = strtok(NULL, " \t");
    
    if (extra) {
        response_set_error(s, i18n_get(s->ui.language, I18N_USAGE_SET_KEY_VALUE));
    } else {
        cmd_set(s, key, val);
    }
}

static void handle_lang(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    cmd_lang(s, args ? args : "");
}

static void handle_layout(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    cmd_layout(s, args ? args : "");
}

static void handle_clear_history(AppState *s, const Keymap *km, const char *args) {
    (void)km;
    (void)args;
    cmd_clear_history(s);
}

/* Command registry */
static const CommandEntry command_registry[] = {
    {"quit", "q", handle_quit},
    {"help", "h", handle_help},
    {"theme", NULL, handle_theme},
    {"export", NULL, handle_export},
    {"auth", NULL, handle_auth},
    {"find", NULL, handle_find},
    {"set", NULL, handle_set},
    {"lang", NULL, handle_lang},
    {"layout", NULL, handle_layout},
    {"clear!", "ch!", handle_clear_history},
    {NULL, NULL, NULL}  /* Sentinel */
};

void command_parse_and_execute(AppState *s, const Keymap *km, const char *cmd) {
    if (!s) return;

    char local[PROMPT_MAX];
    if (!cmd) cmd = "";
    strncpy(local, cmd, sizeof(local) - 1);
    local[sizeof(local) - 1] = '\0';

    /* Skip leading whitespace and colon */
    char *p = local;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == ':') p++;
    while (*p && isspace((unsigned char)*p)) p++;

    /* Trim trailing whitespace */
    size_t n = strlen(p);
    while (n > 0 && isspace((unsigned char)p[n - 1])) {
        p[n - 1] = '\0';
        n--;
    }

    /* Empty command - just return to normal mode */
    if (*p == '\0') {
        finish_command(s);
        return;
    }

    /* Extract command name */
    char *args = p;
    while (*args && !isspace((unsigned char)*args)) args++;
    
    if (*args) {
        *args = '\0';
        args++;
        while (*args && isspace((unsigned char)*args)) args++;
        if (!*args) args = NULL;
    } else {
        args = NULL;
    }

    /* Look up command in registry */
    for (int i = 0; command_registry[i].name != NULL; i++) {
        const CommandEntry *entry = &command_registry[i];
        
        /* Check if command name or alias matches */
        int match = (strcmp(p, entry->name) == 0);
        if (!match && entry->alias && strcmp(p, entry->alias) == 0) {
            match = 1;
        }
        
        if (match) {
            entry->handler(s, km, args);
            finish_command(s);
            return;
        }
    }

    /* Command not found */
    char msg[256];
    snprintf(msg, sizeof(msg), i18n_get(s->ui.language, I18N_UNKNOWN_COMMAND_FMT), p);
    response_set_error(s, msg);
    finish_command(s);
}
