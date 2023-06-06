#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "ui.h"

void login(const char *username, const char *password) { printf("Logging in \"%s\" with password=\"%s\"\n", username, password); }

int main(void) {
    show();

    int running = 1;
    while (running) {
        int ch = getch();
        // TODO: can Ctrl-C or Ctrl-/ happen/affect anything in agetty?
        switch (ch) {
            case '\n':
            case KEY_ENTER:
                const char *username = get_value(0), *password = get_value(1);
                if (strlen(password)) login(username, password);
                break;
            default:
                handle_input(ch);
                break;
        }
    }

    close();
    return EXIT_SUCCESS;
}