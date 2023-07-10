#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include "config.h"
#include "login.h"
#include "ui.h"

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

int main(void) {
    set_signal_handlers();

    openlog("ssdm", LOG_NDELAY, LOG_AUTH);
    if (atexit(closelog) != 0) syslog(LOG_CRIT, "Unable to register \"closelog\" to run atexit");

    load_config();
    if (atexit(free_config) != 0) syslog(LOG_CRIT, "Unable to register \"free_config\" to run atexit");

    open_ui();
    if (atexit(close_ui) != 0) syslog(LOG_CRIT, "Unable to register \"close_ui\" to run atexit");

    focus_tty();

    while (1) {
        refresh_window();
        int ch = get_ch();

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
                login();
                break;
            default:
                append_char(ch);
                break;
        };
    }

    return EXIT_SUCCESS;
}