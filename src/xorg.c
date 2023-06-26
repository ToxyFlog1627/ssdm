#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE
#include <X11/Xlib.h>
#include <assert.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>
#include "pam.h"

#define RUNTIME_DIR_PATH_LENGTH 18

struct passwd *pwd;

void init_env(void) {
    if (getenv("TERM") == NULL) setenv("TERM", "linux", 1);
    if (getenv("LANG") == NULL) setenv("LANG", "C", 1);
    setenv("HOME", pwd->pw_dir, 1);
    setenv("PWD", pwd->pw_dir, 1);
    setenv("SHELL", pwd->pw_shell, 1);
    setenv("USER", pwd->pw_name, 1);
    setenv("LOGNAME", pwd->pw_name, 1);

    char runtime_dir_path[RUNTIME_DIR_PATH_LENGTH + 1];
    snprintf(runtime_dir_path, RUNTIME_DIR_PATH_LENGTH, "/run/user/%d", getuid());
    setenv("XDG_RUNTIME_DIR", runtime_dir_path, 0);
    setenv("XDG_SESSION_CLASS", "user", 0);
    setenv("XDG_SESSION_ID", "1", 0);
    setenv("XDG_SESSION_DESKTOP", "xinitrc", 0);
    setenv("XDG_SESSION_TYPE", "x11", 0);
    setenv("XDG_SEAT", "seat0", 0);
    setenv("XDG_VTNR", ttyname(STDIN_FILENO), 0);
}

void add_utmp_entry(void) {
    struct utmp entry;
    entry.ut_type = USER_PROCESS;
    entry.ut_pid = getpid();
    strcpy(entry.ut_line, ttyname(STDIN_FILENO) + strlen("/dev/"));
    strcpy(entry.ut_id, ttyname(STDIN_FILENO) + strlen("/dev/tty"));
    strcpy(entry.ut_user, pwd->pw_name);
    memset(entry.ut_host, 0, UT_HOSTSIZE);
    time((time_t *) &entry.ut_time);
    entry.ut_addr = 0;

    setutent();
    if (pututline(&entry) == NULL) {
        syslog(LOG_EMERG, "Unable to add utmp entry");
        exit(EXIT_FAILURE);
    }
    endutent();
}

void delete_utmp_entry(void) {
    struct utmp entry;
    entry.ut_type = DEAD_PROCESS;
    entry.ut_pid = 0;
    memset(entry.ut_host, 0, UT_HOSTSIZE);
    memset(entry.ut_user, 0, UT_NAMESIZE);
    entry.ut_time = 0;

    setutent();
    if (pututline(&entry) == NULL) {
        syslog(LOG_EMERG, "Unable to delete utmp entry");
        exit(EXIT_FAILURE);
    }
    endutent();
}

void xorg(void) {
    if (initgroups(pwd->pw_name, pwd->pw_gid) == -1) exit(EXIT_FAILURE);
    if (setgid(pwd->pw_gid) == -1) exit(EXIT_FAILURE);
    if (setuid(pwd->pw_uid) == -1) exit(EXIT_FAILURE);

    init_env();
    pam_init_env();

    // TODO: actually start

    exit(EXIT_SUCCESS);
}

void start_xorg(const char *username) {
    assert(username != NULL && username[0] != '\0');

    pwd = getpwnam(username);
    if (pwd == NULL) {
        syslog(LOG_EMERG, "Unable to get passwd of \"%s\"", username);
        exit(EXIT_FAILURE);
    }
    assert(pwd->pw_shell != NULL && *pwd->pw_shell != '\0');
    endpwent();

    pid_t pid = fork();
    if (pid == -1) {
        syslog(LOG_EMERG, "Unable to fork in order to start xorg");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) xorg();

    add_utmp_entry();
    int status;
    waitpid(pid, &status, 0);
    if (status != 0) syslog(LOG_ALERT, "Unsuccessful return when starting xorg");
    delete_utmp_entry();
}