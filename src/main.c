#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "pam.h"
#include "ui.h"

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
            case AUTH_WRONG_CREDENTIALS:
                show_message("incorrect login/password");
                break;
            case AUTH_ERROR:
                // TODO: implement
                break;
            case AUTH_SUCCESS:
                running = 0;
                break;
            default:
                abort();
                break;
        }
    }

    // TODO: start xorg, open DE/WM

    if (logout() == AUTH_ERROR) {
        // TODO: throw error
    }
    close_ui();

    return EXIT_SUCCESS;
}