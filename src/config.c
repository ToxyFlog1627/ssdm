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

#define SKIP_SPACES(ch) \
    while (*ch == ' ') ch++;

#define PROCESSING_ERROR(error)                                                            \
    {                                                                                      \
        syslog(LOG_WARNING, "Invalid line #%d in config file. It %s", line_number, error); \
        goto exit;                                                                         \
    }
#define INVALID_PROPERTY_ERROR() syslog(LOG_WARNING, "Invalid property \"%s\" in config file", key);

#define IS_TERMINATING_CHAR(ch) ((ch == COMMENT_CHAR) || (ch == '\n') || (ch == '\0'))
#define IS_VALID_KEY_CHAR(ch) ((ch >= 'A' && ch <= 'z') || ch == '_')

#define IS_DIGIT(ch) (ch >= '0' && ch <= '9')
#define IS_BOOL(string) (strcmp(string, "true") == 0 || strcmp(string, "false") == 0)

#define SET_PROPERTY(property)         \
    if (strcmp(key, #property) == 0) { \
        config.property = value;       \
        return;                        \
    }

FILE *config_file;
config_t config;

void set_char_property(char *key, char value) {
    assert(value >= ' ' && value <= '~');

    SET_PROPERTY(password_char);
    SET_PROPERTY(input_placeholder_char);

    INVALID_PROPERTY_ERROR();
}

void set_string_property(char *key, char *value) {
    assert(value != NULL && value[0] != '\0');

    set_char_property(key, *value);
}

void set_number_property(char *key, long int value) {
    SET_PROPERTY(error_message_duration_seconds)

    INVALID_PROPERTY_ERROR();
}

void set_bool_property(char *key, char value) {
    assert(value == 0 || value == 1);

    SET_PROPERTY(erase_password_on_failure);
    SET_PROPERTY(save_login);

    INVALID_PROPERTY_ERROR();
}

char process_line(int line_number) {
    assert(line_number > 0);

    size_t length, i;
    char *line = NULL, *ch, *key = NULL, *value = NULL;
    char is_string = 0;
    if (getline(&line, &length, config_file) == -1) return 0;
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

    SKIP_SPACES(ch);
    if (IS_TERMINATING_CHAR(*ch)) return 1;
    if (assignment_index == -1) PROCESSING_ERROR("doesn't contain assignment character");

    key = (char *) calloc(assignment_index + 1, sizeof(char));
    if (key == NULL) {
        syslog(LOG_EMERG, "Bad malloc of config key");
        exit(EXIT_FAILURE);
    }

    for (i = 0; *ch != ASSIGNMENT_CHAR; ch++) {
        if (*ch == ' ') continue;
        if (IS_TERMINATING_CHAR(*ch)) PROCESSING_ERROR("terminates before assignment character");
        if (!IS_VALID_KEY_CHAR(*ch)) PROCESSING_ERROR("sets property that doesn't exist");
        key[i++] = *ch;
    }
    ch++;  // skip assignment char

    value = (char *) calloc(length - assignment_index, sizeof(char));
    if (value == NULL) {
        syslog(LOG_EMERG, "Bad malloc of config value");
        exit(EXIT_FAILURE);
    }

    for (i = 0; !IS_TERMINATING_CHAR(*ch); ch++) {
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
        set_string_property(key, string);
    } else if (IS_DIGIT(*ch)) {
        char *ch = value;
        while (IS_DIGIT(*ch)) ch++;
        if (*ch != '\0') PROCESSING_ERROR("sets value that neither string nor number")
        long int num = strtol(value, &ch, 10);
        set_number_property(key, num);
    } else if (IS_BOOL(ch)) {
        char bool = strcmp(value, "true") == 0 ? 1 : 0;
        set_bool_property(key, bool);
    } else {
        if (line[length - 1] == '\n') line[length - 1] = '\0';
        syslog(LOG_WARNING, "Invalid line in config file: \"%s\"", line);
    }

exit:
    if (key != NULL) free(key);
    if (value != NULL) free(value);
    return 1;
}

void load_config(void) {
    config.password_char = '*';
    config.input_placeholder_char = ' ';
    config.erase_password_on_failure = 0;
    config.save_login = 1;
    config.error_message_duration_seconds = 3;

    config_file = fopen(CONFIG_PATH, "r+");
    if (config_file == NULL) {
        syslog(LOG_WARNING, "No config file found at \"%s\"", CONFIG_PATH);
        return;
    }

    int line_number = 1;
    while (process_line(line_number)) line_number++;
}