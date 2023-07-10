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
#include "xorg.h"

void signal_handler(int sig) {
    (void) sig;
    exit(EXIT_SUCCESS);
}

void set_signal_handlers(void) {
    struct sigaction action;
    action.sa_handler = signal_handler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);
}

void try_to_logout(void) {
    if (pam_logout() == AUTH_ERROR) syslog(LOG_CRIT, "PAM Authentication error at logout");
}

char try_to_login(void) {
    switch (pam_login(get_value(I_USERNAME), get_value(I_PASSWORD))) {
        case AUTH_SUCCESS:
            clear_input(I_PASSWORD);
            if (config.save_login) store("username", get_value(I_USERNAME), sizeof(char) * (strlen(get_value(I_USERNAME)) + 1));
            return 1;
        case AUTH_WRONG_CREDENTIALS:
            show_message("incorrect login/password");
            if (config.erase_password_on_failure) clear_input(I_PASSWORD);
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

int main(void) {
    set_signal_handlers();
    openlog("ssdm", LOG_NDELAY, LOG_AUTH);
    load_config();
    open_ui();
    if (atexit(free_config) != 0) syslog(LOG_CRIT, "Unable to register \"free_config\" to run atexit");

    if (atexit(closelog) != 0) syslog(LOG_CRIT, "Unable to register \"closelog\" to run atexit");
    if (atexit(close_ui) != 0) syslog(LOG_CRIT, "Unable to register \"close_ui\" to run atexit");
    if (atexit(try_to_logout) != 0) syslog(LOG_CRIT, "Unable to register \"try_to_logout\" to run atexit");

    if (config.save_login == 1) {
        void *value = load("username");
        if (value != NULL) {
            set_value(I_USERNAME, (char *) value);
            free(value);
            next_input();
            refresh_window();
        }
    }

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
                if (system(config.shutdown_cmd) != 0) syslog(LOG_CRIT, "Unable to shutdown");
                exit(EXIT_SUCCESS);
            case REBOOT_KEY:
                if (system(config.reboot_cmd) != 0) syslog(LOG_CRIT, "Unable to reboot");
                exit(EXIT_SUCCESS);
            case '\n':
            case KEY_ENTER:
                if (!try_to_login()) break;
                start_xorg(get_value(I_USERNAME));
                pam_logout();
                break;
            default:
                append_char(ch);
                break;
        };
        refresh_window();
    }

    return EXIT_SUCCESS;
}