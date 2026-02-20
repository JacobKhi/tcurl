#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();
    
    printw("Key Code Tester - Press keys to see their codes\n");
    printw("Press 'q' to quit\n");
    
    int ch;
    while ((ch = getch()) != 'q') {
        clear();
        printw("Key Code Tester - Press keys to see their codes\n");
        printw("Press 'q' to quit\n");
        printw("Try: Ctrl+Left, Ctrl+Right, Alt+Left, Alt+Right\n");
        printw("     Home, End, Delete\n\n");
        
        printw("Last key pressed:\n");
        printw("  Decimal code: %d\n", ch);
        printw("  Octal code: %o\n", ch);
        printw("  Hex code: 0x%x\n", ch);
        
        if (ch >= 32 && ch <= 126) {
            printw("  Character: '%c'\n", ch);
        }
        
        const char *keyname_str = keyname(ch);
        if (keyname_str) {
            printw("  Key name: %s\n", keyname_str);
        }
        
        // Common key codes for reference
        if (ch == KEY_LEFT) printw("  -> KEY_LEFT\n");
        if (ch == KEY_RIGHT) printw("  -> KEY_RIGHT\n");
        if (ch == KEY_HOME) printw("  -> KEY_HOME\n");
        if (ch == KEY_END) printw("  -> KEY_END\n");
        if (ch == KEY_DC) printw("  -> KEY_DC (Delete)\n");
        if (ch == 546) printw("  -> Possible Ctrl+Left\n");
        if (ch == 561) printw("  -> Possible Ctrl+Right\n");
        if (ch == 27) printw("  -> ESC or Alt sequence start\n");
        
        refresh();
    }
    
    endwin();
    
    printf("\nKey tester exited.\n");

    return 0;
}
