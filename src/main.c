#include <locale.h>
#include <ncurses.h>
#include <curl/curl.h>

#include "state.h"
#include "core/keymap.h"
#include "ui/draw.h"
#include "ui/input.h"

void dispatch_action(AppState *s, Action a);

int main(void) {
    setlocale(LC_ALL, "");

    AppState state;
    app_state_init(&state);

    Keymap keymap;
    (void)keymap_load_file(&keymap, "config/keymap.conf");

    initscr();
    set_escdelay(25);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    timeout(100);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    while (state.running) {
        ui_draw(&state);

        int ch = getch();
        if (ch == ERR) continue;

        ui_handle_key(&state, &keymap, ch);
    }

    endwin();
    app_state_destroy(&state);
    curl_global_cleanup();
    return 0;
}
