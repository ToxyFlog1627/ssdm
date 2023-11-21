#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include "config.h"
#include "pam.h"
#include "store.h"
#include "ui.h"
#include "xorg.h"

#define MAX_RUNTIME_DIR_PATH_LENGTH 31

#define SETENV(var, value, overwrite) \
    if (setenv(var, value, overwrite) == -1) syslog(LOG_ALERT, "Unable to set %s to \"%s\"", var, value);

static void init_env(const char *username) {
    struct passwd *pwd = getpwnam(username);

    if (getenv("TERM") == NULL) SETENV("TERM", "linux", 1);
    if (getenv("LANG") == NULL) SETENV("LANG", "C", 1);
    SETENV("HOME", pwd->pw_dir, 1);
    SETENV("PWD", pwd->pw_dir, 1);
    SETENV("SHELL", pwd->pw_shell, 1);
    SETENV("USER", pwd->pw_name, 1);
    SETENV("LOGNAME", pwd->pw_name, 1);
    SETENV("DISPLAY", ":0", 1);

    char runtime_dir_path[MAX_RUNTIME_DIR_PATH_LENGTH + 1];
    if (snprintf(runtime_dir_path, MAX_RUNTIME_DIR_PATH_LENGTH, "/run/user/%d", pwd->pw_uid) < 0) syslog(LOG_ALERT, "Unable to concatenate runtime dir path");
    else SETENV("XDG_RUNTIME_DIR", runtime_dir_path, 0);
    SETENV("XDG_SESSION_CLASS", "user", 0);
    SETENV("XDG_SESSION_ID", "1", 0);
    SETENV("XDG_SESSION_DESKTOP", "xinitrc", 0);
    SETENV("XDG_SESSION_TYPE", "x11", 0);
    SETENV("XDG_SEAT", "seat0", 0);
    SETENV("XDG_VTNR", ttyname(STDIN_FILENO), 0);
}

static void try_to_logout(void) {
    if (pam_logout() == AUTH_ERROR) syslog(LOG_ALERT, "PAM Authentication error at logout");
}

static char try_to_login(const char *username, const char *password) {
    clearenv();
    init_env(username);

    switch (pam_login(username, password)) {
        case AUTH_SUCCESS:
            clear_input(I_PASSWORD);
            if (config.save_login) store("username", username, sizeof(char) * (strlen(username) + 1));
            return 1;
        case AUTH_WRONG_CREDENTIALS:
            show_message("incorrect login/password");
            if (config.erase_password_on_failure) clear_input(I_PASSWORD);
            break;
        case AUTH_TOO_MANY_ATTEMPTS:
            show_message("too many failed attempts, try later");
            if (config.erase_password_on_failure) clear_input(I_PASSWORD);
            break;
        case AUTH_ERROR:
            syslog(LOG_ERR, "PAM Authentication error at login");
            break;
        default:
            syslog(LOG_ALERT, "Unkown return value from login");
            break;
    }

    clearenv();
    return 0;
}

void login(void) {
    if (!try_to_login(get_value(I_USERNAME), get_value(I_PASSWORD))) return;
    if (atexit(try_to_logout) != 0) syslog(LOG_ALERT, "Unable to register \"try_to_logout\" to run atexit");

    alarm(0);
    pam_init_env();
    start_xorg(get_value(I_USERNAME));
    clearenv();

    try_to_logout();
    if (system("loginctl terminate-session self") == -1) syslog(LOG_ALERT, "Unable to terminate session!");
}