#ifndef CONFIG_H
#define CONFIG_H

typedef struct config_t {
    char password_char, input_placeholder_char;
    char erase_password_on_failure, save_login;
    int error_message_duration_seconds;
} config_t;

extern config_t config;

void load_config(void);

#endif  // CONFIG_H