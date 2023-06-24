#define _POSIX_C_SOURCE 200809L
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include "config.h"
#include "pam.h"
#include "store.h"
#include "ui.h"
#include "utils.h"

void signal_handler(int sig) {
    (void) sig;
    exit(EXIT_SUCCESS);
}

void try_to_logout(void) {
    if (logout() == AUTH_ERROR) syslog(LOG_CRIT, "PAM Authentication error at logout");
}

char try_to_login(void) {
    switch (login(get_value(I_LOGIN), get_value(I_PASSWORD))) {
        case AUTH_SUCCESS:
            if (atexit(try_to_logout) != 0) syslog(LOG_CRIT, "Unable to register \"try_to_logout\" to run atexit");
            if (config.save_login) store("login", get_value(I_LOGIN), sizeof(char) * (strlen(get_value(I_LOGIN)) + 1));
            return 1;
        case AUTH_WRONG_CREDENTIALS:
            show_message("incorrect login/password");
            if (config.erase_password_on_failure) reset_password();
            break;
        case AUTH_ERROR:
            syslog(LOG_ERR, "PAM Authentication error at login");
            break;
        default:
            syslog(LOG_EMERG, "Unkown return value from login");
            exit(EXIT_FAILURE);
            break;
    }

    return 0;
}

void handle_login(void) {
    while (1) {
        int ch = getch();
        switch (ch) {
            case KEY_BTAB:
            case KEY_UP:
                prev_input();
                break;
            case '\t':
            case KEY_DOWN:
                next_input();
                break;
            case 127:
            case '\b':
            case KEY_BACKSPACE:
                delete_char();
                break;
            case SHUTDOWN_KEY:
                if (system("poweroff") != 0) syslog(LOG_CRIT, "Unable to shutdown");
                exit(EXIT_SUCCESS);
            case REBOOT_KEY:
                if (system("reboot") != 0) syslog(LOG_CRIT, "Unable to reboot");
                exit(EXIT_SUCCESS);
            case '\n':
            case KEY_ENTER:
                if (try_to_login()) return;
                break;
            default:
                append_char(ch);
                break;
        };
        refresh_window();
    }
}

int main(void) {
    struct sigaction action;
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    openlog("ssdm", LOG_NDELAY, LOG_AUTH);
    if (atexit(closelog) != 0) syslog(LOG_CRIT, "Unable to register \"closelog\" to run atexit");

    load_config();

    open_ui();
    if (atexit(close_ui) != 0) syslog(LOG_CRIT, "Unable to register \"close_ui\" to run atexit");

    if (config.save_login == 1) {
        void *value = load("login");
        if (value != NULL) set_value(I_LOGIN, (char *) value);
        free(value);
        refresh_window();
    }

    handle_login();

    // TODO: start xorg, open DE/WM

    return EXIT_SUCCESS;
}