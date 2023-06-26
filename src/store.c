#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <unistd.h>

#define STORE_DIR_PATH "/var/lib/ssdm"
#define STORE_DIR_PERMISSIONS S_IRUSR | S_IWUSR | S_IXUSR
#define STORE_FILE_PERMISSIONS S_IRUSR | S_IWUSR

#define ERROR_FD -2

int dir_fd = -1;

void open_store_dir(void) {
    dir_fd = open(STORE_DIR_PATH, O_RDONLY, O_DIRECTORY);
    if (dir_fd != -1) return;

    if (errno != ENOENT) {
        syslog(LOG_CRIT, "Unable to open store directory %d", errno);
        dir_fd = -2;
        return;
    }

    if (mkdir(STORE_DIR_PATH, STORE_DIR_PERMISSIONS) == -1) {
        syslog(LOG_CRIT, "Unable to create store directory");
        dir_fd = -2;
        return;
    }

    open_store_dir();
}

char store(const char *key, const void *value, size_t size) {
    assert(key != NULL && key[0] != '\0' && size > 0);

    if (dir_fd == ERROR_FD) return -1;
    if (dir_fd == -1) open_store_dir();

    int fd = openat(dir_fd, key, O_WRONLY | O_CREAT | O_TRUNC, STORE_FILE_PERMISSIONS);
    if (fd == -1) {
        syslog(LOG_CRIT, "Unable to open file to store \"%s\"", key);
        dir_fd = -2;
        return -1;
    }

    if (write(fd, &size, sizeof(size)) == -1) {
        syslog(LOG_CRIT, "Unable to store size of \"%s\"", key);
        dir_fd = -2;
        return -1;
    }

    if (write(fd, value, size) == -1) {
        syslog(LOG_CRIT, "Unable to store \"%s\"", key);
        dir_fd = -2;
        return -1;
    }

    return 0;
}

void *load(const char *key) {
    assert(key != NULL && key[0] != '\0');

    if (dir_fd == ERROR_FD) return NULL;
    if (dir_fd == -1) open_store_dir();

    int fd = openat(dir_fd, key, O_RDONLY);
    if (fd == -1) return NULL;

    size_t size;
    if (read(fd, &size, sizeof(size)) == -1) {
        syslog(LOG_CRIT, "Unable to load size of \"%s\"", key);
        dir_fd = -2;
        return NULL;
    }

    void *value = malloc(size);
    if (value == NULL) {
        syslog(LOG_EMERG, "Bad malloc of \"%s\" during load", key);
        exit(EXIT_FAILURE);
    }

    if (read(fd, value, size) == -1) {
        syslog(LOG_CRIT, "Unable to load \"%s\"", key);
        dir_fd = -2;
        return NULL;
    }

    return value;
}