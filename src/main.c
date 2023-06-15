#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include "pam.h"
#include "ui.h"
#include "utils.h"

int main(void) {
    int logging_options = LOG_NDELAY;
    IF_DEBUGGING(logging_options |= LOG_PERROR)
    openlog("ssdm", logging_options, LOG_AUTH);

    atexit(closelog);
    open_ui();
    atexit(close_ui);

    char running = 1;
    while (running) {
        int ch = getch();
        if (ch != KEY_ENTER && ch != '\n') {
            handle_input(ch);
            continue;
        }

        switch (login(get_value(I_USERNAME), get_value(I_PASSWORD))) {
            case AUTH_SUCCESS:
                running = 0;
                break;
            case AUTH_WRONG_CREDENTIALS:
                show_message("incorrect login/password");
                break;
            case AUTH_ERROR:
                syslog(LOG_ERR, "PAM Authentication error at login");
                break;
            default:
                syslog(LOG_ALERT, "Unkown return value from login");
                exit(EXIT_FAILURE);
                break;
        }
    }

    // TODO: start xorg, open DE/WM

    if (logout() == AUTH_ERROR) syslog(LOG_ERR, "PAM Authentication error at logout");

    return EXIT_SUCCESS;
}