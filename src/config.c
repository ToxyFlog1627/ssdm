#define _GNU_SOURCE
#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>

#define CONFIG_PATH "/etc/ssdm.conf"

#define COMMENT_CHAR '#'
#define ASSIGNMENT_CHAR '='

#define PROCESSING_ERROR(error)                                                            \
    {                                                                                      \
        syslog(LOG_WARNING, "Invalid line #%d in config file. It %s", line_number, error); \
        goto exit;                                                                         \
    }

#define CHECK_STRING_VALUE(property)                                       \
    {                                                                      \
        if (((char *) config.property) == NULL) {                          \
            syslog(LOG_EMERG, "Bad malloc of value of \"%s\"", #property); \
            exit(EXIT_FAILURE);                                            \
        }                                                                  \
    }

#define SET_PROPERTY(property, type)                               \
    if (strcmp(key, #property) == 0) {                             \
        if (type == 'c') {                                         \
            char ch = **((char **) value);                         \
            assert(ch >= ' ' && ch <= '~');                        \
            config.property = ch;                                  \
        } else if (type == 's') {                                  \
            free(config.property);                                 \
            char *str = strdup(*((char **) value));                \
            CHECK_STRING_VALUE(property);                          \
            config.property = str;                                 \
        } else if (type == 'n') {                                  \
            config.property = *((long int *) value);               \
        } else if (type == 'b') {                                  \
            char bool = *((char *) value);                         \
            assert(bool == 0 || bool == 1);                        \
            config.property = bool;                                \
        } else {                                                   \
            syslog(LOG_EMERG, "Unknown property type '%c'", type); \
            exit(EXIT_FAILURE);                                    \
        }                                                          \
        return;                                                    \
    }

config_t config;

static char is_terminating_ch(char ch) { return (ch == COMMENT_CHAR) || (ch == '\n') || (ch == '\0'); }
static char is_valid_key_ch(char ch) { return (ch >= 'A' && ch <= 'z') || ch == '_'; }
static char is_digit(char ch) { return (ch >= '0' && ch <= '9'); }
static char is_bool(char *str) { return (strcmp(str, "true") == 0 || strcmp(str, "false") == 0); }

static void set_property(char *key, void *value) {
    SET_PROPERTY(erase_password_on_failure, 'b');
    SET_PROPERTY(error_message_duration_seconds, 'n');
    SET_PROPERTY(input_placeholder_char, 'c');
    SET_PROPERTY(password_char, 'c');
    SET_PROPERTY(reboot_cmd, 's');
    SET_PROPERTY(save_login, 'b');
    SET_PROPERTY(shutdown_cmd, 's');
    SET_PROPERTY(xauth_filename, 's');
    syslog(LOG_WARNING, "Invalid property \"%s\" in config file", key);
}

static char process_line(FILE *config_file, int line_number) {
    assert(line_number > 0);

    size_t length, i;
    char *line = NULL, *ch, *key = NULL, *value = NULL;
    char is_string = 0;
    if (getline(&line, &length, config_file) == -1) {
        free(line);
        return 0;
    }
    ch = line;
    length = strlen(line);

    int assignment_index = -1;
    for (i = 0; i < length; i++) {
        if (line[i] == '"') is_string = !is_string;
        if (is_string || line[i] != ASSIGNMENT_CHAR) continue;
        if (assignment_index != -1) PROCESSING_ERROR("contains more than one assignment character");
        assignment_index = i;
    }
    if (is_string) PROCESSING_ERROR("contains unmatched quotes");

    while (*ch == ' ') ch++;
    if (is_terminating_ch(*ch)) goto exit;
    if (assignment_index == -1) PROCESSING_ERROR("doesn't contain assignment character");

    key = (char *) calloc(assignment_index + 1, sizeof(char));
    if (key == NULL) {
        syslog(LOG_EMERG, "Bad malloc of config key");
        exit(EXIT_FAILURE);
    }

    for (i = 0; *ch != ASSIGNMENT_CHAR; ch++) {
        if (*ch == ' ') continue;
        if (is_terminating_ch(*ch)) PROCESSING_ERROR("terminates before assignment character");
        if (!is_valid_key_ch(*ch)) PROCESSING_ERROR("sets property that doesn't exist");
        key[i++] = *ch;
    }
    ch++;  // skip assignment char

    value = (char *) calloc(length - assignment_index, sizeof(char));
    if (value == NULL) {
        syslog(LOG_EMERG, "Bad malloc of config value");
        exit(EXIT_FAILURE);
    }

    for (i = 0; !is_terminating_ch(*ch); ch++) {
        if (*ch == ' ' && !is_string) continue;
        if (*ch == '"') {
            if (is_string) break;
            is_string = 1;
        }
        value[i++] = *ch;
    }

    ch = value;
    if (*ch == '"') {
        char *string = ch + 1;
        set_property(key, (void *) &string);
    } else if (is_digit(*ch)) {
        char *ch = value;
        while (is_digit(*ch)) ch++;
        if (*ch != '\0') PROCESSING_ERROR("sets value that neither string nor number")
        long int num = strtol(value, NULL, 10);
        set_property(key, (void *) &num);
    } else if (is_bool(ch)) {
        char bool = strcmp(value, "true") == 0 ? 1 : 0;
        set_property(key, (void *) &bool);
    } else {
        if (line[length - 1] == '\n') line[length - 1] = '\0';
        syslog(LOG_WARNING, "Invalid line in config file: \"%s\"", line);
    }

exit:
    free(line);
    if (key != NULL) free(key);
    if (value != NULL) free(value);
    return 1;
}

void load_config(void) {
    config.erase_password_on_failure = 0;
    config.error_message_duration_seconds = 3;
    config.input_placeholder_char = ' ';
    config.password_char = '*';
    config.reboot_cmd = strdup("loginctl reboot");
    config.save_login = 1;
    config.shutdown_cmd = strdup("loginctl poweroff");
    config.xauth_filename = strdup("ssdm_xauth");

    FILE *config_file = fopen(CONFIG_PATH, "r+");
    if (config_file == NULL) {
        syslog(LOG_WARNING, "No config file found at \"%s\"", CONFIG_PATH);
        return;
    }

    int line_number = 1;
    while (process_line(config_file, line_number)) line_number++;

    CHECK_STRING_VALUE(reboot_cmd);
    CHECK_STRING_VALUE(shutdown_cmd);
    CHECK_STRING_VALUE(xauth_filename);
}

void free_config(void) {
    free(config.reboot_cmd);
    free(config.shutdown_cmd);
    free(config.xauth_filename);
}