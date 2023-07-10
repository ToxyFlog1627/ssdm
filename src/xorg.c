#define _DEFAULT_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <utmp.h>
#include "config.h"

#define MAX_COMMAND_LENGTH 512
#define XSETUP_PATH "/usr/share/ssdm/xsetup.sh"

#define MAX_XAUTHORITY_PATH_LENGTH 255
#define XAUTHORITY_FILE_PERMISSIONS S_IRUSR | S_IWUSR

#define CREATE_FILE(path, permissions)                               \
    {                                                                \
        int fd = open(path, O_RDWR | O_CREAT, permissions);          \
        if (fd == -1 || close(fd) == -1) {                           \
            syslog(LOG_EMERG, "Unable to create file \"%s\"", path); \
            _exit(EXIT_FAILURE);                                     \
        }                                                            \
    }

#define EXEC(pid, ...)                                                      \
    {                                                                       \
        char command[MAX_COMMAND_LENGTH + 1];                               \
        snprintf(command, MAX_COMMAND_LENGTH + 1, __VA_ARGS__);             \
        pid = fork();                                                       \
        if (pid == -1) {                                                    \
            syslog(LOG_EMERG, "Unable to fork to execute \"%s\"", command); \
            _exit(EXIT_FAILURE);                                            \
        }                                                                   \
        if (pid == 0) {                                                     \
            execl(pwd->pw_shell, pwd->pw_shell, "-c", command, NULL);       \
            _exit(EXIT_SUCCESS);                                            \
        }                                                                   \
    }
#define WAIT_FOR(pid, ...)                  \
    {                                       \
        int status;                         \
        waitpid(pid, &status, 0);           \
        if (status != 0) {                  \
            syslog(LOG_EMERG, __VA_ARGS__); \
            _exit(EXIT_FAILURE);            \
        }                                   \
    }
#define EXEC_AND_WAIT(...)                                                                      \
    {                                                                                           \
        char command[MAX_COMMAND_LENGTH + 1];                                                   \
        snprintf(command, MAX_COMMAND_LENGTH + 1, __VA_ARGS__);                                 \
                                                                                                \
        pid_t pid = fork();                                                                     \
        if (pid == -1) {                                                                        \
            syslog(LOG_EMERG, "Unable to fork to execute \"%s\"", command);                     \
            _exit(EXIT_FAILURE);                                                                \
        }                                                                                       \
        if (pid == 0) {                                                                         \
            execl(pwd->pw_shell, pwd->pw_shell, "-c", command, NULL);                           \
            _exit(EXIT_SUCCESS);                                                                \
        }                                                                                       \
                                                                                                \
        WAIT_FOR(pid, "Process executing \"%s\" exited with error code = %d", command, status); \
    }

struct passwd* pwd;

void add_utmp_entry(void) {
    struct utmp entry;
    entry.ut_type = USER_PROCESS;
    entry.ut_pid = getpid();
    strcpy(entry.ut_line, ttyname(STDIN_FILENO) + strlen("/dev/"));
    strcpy(entry.ut_id, ttyname(STDIN_FILENO) + strlen("/dev/tty"));
    strcpy(entry.ut_user, pwd->pw_name);
    memset(entry.ut_host, 0, UT_HOSTSIZE);
    time((time_t*) &entry.ut_time);
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
    if (chdir(pwd->pw_dir) == -1) _exit(EXIT_FAILURE);
    if (initgroups(pwd->pw_name, pwd->pw_gid) == -1) _exit(EXIT_FAILURE);
    if (setgid(pwd->pw_gid) == -1) _exit(EXIT_FAILURE);
    if (setuid(pwd->pw_uid) == -1) _exit(EXIT_FAILURE);

    char xauthority[MAX_XAUTHORITY_PATH_LENGTH + 1];
    snprintf(xauthority, MAX_XAUTHORITY_PATH_LENGTH, "%s/%s", getenv("XDG_RUNTIME_DIR"), config.xauth_filename);

    setenv("XAUTHORITY", xauthority, 1);
    CREATE_FILE(xauthority, XAUTHORITY_FILE_PERMISSIONS);
    EXEC_AND_WAIT("xauth add :0 . $(mcookie)");

    int null_fd = open("/dev/null", O_WRONLY);
    if (null_fd == -1) {
        syslog(LOG_ALERT, "Unable to open /dev/null!");
    } else {
        if (dup2(null_fd, STDOUT_FILENO) == -1) syslog(LOG_ALERT, "Unable to redirect Xorg's stdout to /dev/null!");
        if (dup2(null_fd, STDERR_FILENO) == -1) syslog(LOG_ALERT, "Unable to redirect Xorg's stderr to /dev/null!");
    }

    pid_t xorg_pid;
    EXEC(xorg_pid, "X :0 vt%s", ttyname(STDIN_FILENO) + strlen("/dev/tty"));

    pid_t xinitrc_pid;
    EXEC(xinitrc_pid, "%s ~/.xinitrc", XSETUP_PATH);

    int status;
    waitpid(xinitrc_pid, &status, 0);
    if (status != 0) syslog(LOG_EMERG, "Xinitrc exited with error code");

    kill(xorg_pid, SIGTERM);

    EXEC_AND_WAIT("xauth remove :0");
    _exit(EXIT_SUCCESS);
}

void start_xorg(const char* username) {
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

    add_utmp_entry();
    if (pid == 0) xorg();

    int status;
    waitpid(pid, &status, 0);
    if (status != 0) syslog(LOG_ALERT, "Xorg-launching process exited with error code = %d", status);
    delete_utmp_entry();
}