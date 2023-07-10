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

void init_env(const char *username) {
    struct passwd *pwd = getpwnam(username);

    if (getenv("TERM") == NULL) setenv("TERM", "linux", 1);
    if (getenv("LANG") == NULL) setenv("LANG", "C", 1);
    setenv("HOME", pwd->pw_dir, 1);
    setenv("PWD", pwd->pw_dir, 1);
    setenv("SHELL", pwd->pw_shell, 1);
    setenv("USER", pwd->pw_name, 1);
    setenv("LOGNAME", pwd->pw_name, 1);
    setenv("DISPLAY", ":0", 1);

    char runtime_dir_path[MAX_RUNTIME_DIR_PATH_LENGTH + 1];
    snprintf(runtime_dir_path, MAX_RUNTIME_DIR_PATH_LENGTH, "/run/user/%d", getuid());
    setenv("XDG_RUNTIME_DIR", runtime_dir_path, 0);
    setenv("XDG_SESSION_CLASS", "user", 0);
    setenv("XDG_SESSION_ID", "1", 0);
    setenv("XDG_SESSION_DESKTOP", "xinitrc", 0);
    setenv("XDG_SESSION_TYPE", "x11", 0);
    setenv("XDG_SEAT", "seat0", 0);
    setenv("XDG_VTNR", ttyname(STDIN_FILENO), 0);
}

void try_to_logout(void) {
    if (pam_logout() == AUTH_ERROR) syslog(LOG_CRIT, "PAM Authentication error at logout");
}

char try_to_login(const char *username, const char *password) {
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
        case AUTH_ERROR:
            syslog(LOG_ERR, "PAM Authentication error at login");
            break;
        default:
            syslog(LOG_EMERG, "Unkown return value from login");
            exit(EXIT_FAILURE);
            break;
    }

    clearenv();
    return 0;
}

void login(void) {
    const char *username = strdup(get_value(I_USERNAME));  // input values get cleaned on close_ui
    if (!try_to_login(username, get_value(I_PASSWORD))) return;
    if (atexit(try_to_logout) != 0) syslog(LOG_CRIT, "Unable to register \"logout\" to run atexit");
    pam_init_env();

    close_ui();
    start_xorg(username);
    open_ui();

    try_to_logout();
    free(username);
}