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

#define MAKE_COMMAND(command_var, ...)                                    \
    char command_var[MAX_COMMAND_LENGTH + 1];                             \
    if (snprintf(command_var, MAX_COMMAND_LENGTH + 1, __VA_ARGS__) < 0) { \
        syslog(LOG_EMERG, "Unable to concatenate command");               \
        _exit(EXIT_FAILURE);                                              \
    }

#define SYSTEM(...)                                                                                   \
    {                                                                                                 \
        MAKE_COMMAND(command, __VA_ARGS__);                                                           \
        pid_t pid = exec(command);                                                                    \
                                                                                                      \
        int status;                                                                                   \
        waitpid(pid, &status, 0);                                                                     \
        if (status != 0) {                                                                            \
            syslog(LOG_ALERT, "Process running \"%s\" exited with error code = %d", command, status); \
            _exit(EXIT_FAILURE);                                                                      \
        }                                                                                             \
    }

static char* tty;
static struct passwd* pwd;

static pid_t exec(char* command) {
    pid_t pid = fork();
    if (pid == -1) {
        syslog(LOG_EMERG, "Unable to fork to execute \"%s\"", command);
        _exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        execl(pwd->pw_shell, pwd->pw_shell, "-c", command, NULL);
        syslog(LOG_EMERG, "Error executing \"%s\"", command);
        _exit(EXIT_FAILURE);
    }
    return pid;
}

static void create_file(char* path, int permissions) {
    int fd = open(path, O_RDWR | O_CREAT, permissions);
    if (fd == -1 || close(fd) == -1) {
        syslog(LOG_ALERT, "Unable to create file \"%s\"", path);
        _exit(EXIT_FAILURE);
    }
}

static void add_utmp_entry(void) {
    struct utmp entry;
    entry.ut_type = USER_PROCESS;
    entry.ut_pid = getpid();
    strcpy(entry.ut_line, tty + strlen("/dev/"));
    strcpy(entry.ut_id, tty + strlen("/dev/tty"));
    strcpy(entry.ut_user, pwd->pw_name);
    memset(entry.ut_host, 0, UT_HOSTSIZE);
    time((time_t*) &entry.ut_time);
    entry.ut_addr = 0;

    setutent();
    if (pututline(&entry) == NULL) {
        syslog(LOG_ALERT, "Unable to add utmp entry");
        exit(EXIT_FAILURE);
    }
    endutent();
}

static void delete_utmp_entry(void) {
    struct utmp entry;
    entry.ut_type = DEAD_PROCESS;
    entry.ut_pid = 0;
    memset(entry.ut_host, 0, UT_HOSTSIZE);
    memset(entry.ut_user, 0, UT_NAMESIZE);
    entry.ut_time = 0;

    setutent();
    if (pututline(&entry) == NULL) {
        syslog(LOG_ALERT, "Unable to delete utmp entry");
        exit(EXIT_FAILURE);
    }
    endutent();
}

static void xorg(void) {
    if (chdir(pwd->pw_dir) == -1) {
        syslog(LOG_ALERT, "Unable to change directory to \"%s\"", pwd->pw_dir);
        _exit(EXIT_FAILURE);
    }
    if (initgroups(pwd->pw_name, pwd->pw_gid) == -1) {
        syslog(LOG_ALERT, "Unable to init user's groups");
        _exit(EXIT_FAILURE);
    }
    if (setgid(pwd->pw_gid) == -1) {
        syslog(LOG_ALERT, "Unable to change group id");
        _exit(EXIT_FAILURE);
    }
    if (setuid(pwd->pw_uid) == -1) {
        syslog(LOG_ALERT, "Unable to change user id");
        _exit(EXIT_FAILURE);
    }

    char xauthority[MAX_XAUTHORITY_PATH_LENGTH + 1];
    snprintf(xauthority, MAX_XAUTHORITY_PATH_LENGTH, "%s/%s", getenv("XDG_RUNTIME_DIR"), config.xauth_filename);

    if (setenv("XAUTHORITY", xauthority, 1) == -1) {
        syslog(LOG_ALERT, "Unable to set XAUTHORITY to \"%s\"", xauthority);
        _exit(EXIT_FAILURE);
    }
    create_file(xauthority, XAUTHORITY_FILE_PERMISSIONS);
    SYSTEM("xauth add :0 . $(mcookie)");

    int null_fd = open("/dev/null", O_WRONLY);
    if (null_fd == -1) {
        syslog(LOG_ERR, "Unable to open /dev/null!");
    } else {
        if (dup2(null_fd, STDOUT_FILENO) == -1) syslog(LOG_ERR, "Unable to redirect Xorg's stdout to /dev/null!");
        if (dup2(null_fd, STDERR_FILENO) == -1) syslog(LOG_ERR, "Unable to redirect Xorg's stderr to /dev/null!");
    }

    MAKE_COMMAND(xorg_command, "X :0 vt%s", tty + strlen("/dev/tty"));
    pid_t xorg_pid = exec(xorg_command);
    MAKE_COMMAND(xinitrc_command, "%s ~/.xinitrc", XSETUP_PATH);
    pid_t xinitrc_pid = exec(xinitrc_command);

    int status;
    waitpid(xinitrc_pid, &status, 0);
    if (status != 0) syslog(LOG_ERR, "Xinitrc exited with error code");

    kill(xorg_pid, SIGTERM);

    SYSTEM("xauth remove :0");
    if (remove(xauthority) == -1) syslog(LOG_ALERT, "Unable to delete file \"%s\"", xauthority);
    _exit(EXIT_SUCCESS);
}

void start_xorg(const char* username) {
    assert(username != NULL && username[0] != '\0');

    tty = ttyname(STDIN_FILENO);
    if (tty == NULL) {
        syslog(LOG_EMERG, "Unable to get tty");
        exit(EXIT_FAILURE);
    }

    pwd = getpwnam(username);
    if (pwd == NULL) {
        syslog(LOG_EMERG, "Unable to get passwd of \"%s\"", username);
        exit(EXIT_FAILURE);
    }
    assert(pwd->pw_shell != NULL && *pwd->pw_shell != '\0');
    endpwent();

    add_utmp_entry();

    pid_t pid = fork();
    if (pid == -1) {
        syslog(LOG_EMERG, "Unable to fork in order to start xorg");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) xorg();

    int status;
    waitpid(pid, &status, 0);
    if (status != 0) syslog(LOG_ERR, "Xorg-launching process exited with error code = %d", status);

    delete_utmp_entry();
}