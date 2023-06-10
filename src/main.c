#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "ui.h"

void login(const char *username, const char *password) { printf("Logging in \"%s\" with password=\"%s\"\n", username, password); }

int main(void) {
    open_ui();

    // TODO: can Ctrl-C or Ctrl-/ happen/affect anything in agetty?
    char running = 1;
    while (running) {
        int ch = getch();
        if (ch != KEY_ENTER && ch != '\n') {
            handle_input(ch);
            continue;
        }

        switch (login(get_value(I_USERNAME), get_value(I_PASSWORD))) {
                break;
            default:
                abort();
                break;
        }
    }

    close_ui();

    return EXIT_SUCCESS;
}