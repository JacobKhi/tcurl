#include <locale.h>
#include <ncurses.h>
#include <stdio.h>
#include <string.h>

#include "state.h"
#include "core/keymap.h"
#include "core/actions.h"

void dispatch_action(AppState *s, Action a);

static const char *mode_label(Mode m) {
    switch (m) {
        case MODE_NORMAL:  return "NORMAL";
        case MODE_INSERT:  return "INSERT";
        case MODE_COMMAND: return "COMMAND";
        case MODE_SEARCH:  return "SEARCH";
        default:           return "UNKNOWN";
    }
}

static const char *panel_label(Panel p) {
    switch (p) {
        case PANEL_HISTORY:  return "HISTORY";
        case PANEL_EDITOR:   return "EDITOR";
        case PANEL_RESPONSE: return "RESPONSE";
        default:             return "UNKNOWN";
    }
}

static void draw_ui(const AppState *state) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    erase();
    box(stdscr, 0, 0);

    const char *top = " tcurl ";
    mvaddnstr(0, 2, top, cols - 4);

    const char *center = "Keymap-driven TUI";
    int cx = (cols - (int)strlen(center)) / 2;
    if (cx < 1) cx = 1;
    mvaddnstr(rows / 2, cx, center, cols - 2);

    char status[256];
    snprintf(
        status, sizeof(status),
        " %s | focus=%s | history_selected=%d ",
        mode_label(state->mode),
        panel_label(state->focused_panel),
        state->history_selected
    );
    mvaddnstr(rows - 1, 2, status, cols - 4);

    refresh();
}

int main(void) {
    setlocale(LC_ALL, "");

    AppState state;
    app_state_init(&state);

    Keymap keymap;
    (void)keymap_load_file(&keymap, "config/keymap.conf");

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    while (state.running) {
        draw_ui(&state);

        int ch = getch();
        if (ch == ERR) continue;

        Action a = keymap_resolve(&keymap, state.mode, ch);
        if (a != ACT_NONE) {
            dispatch_action(&state, a);
        }
    }

    endwin();
    return 0;
}
