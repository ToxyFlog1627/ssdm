#ifndef CONFIG_H
#define CONFIG_H

typedef struct config_t {
    char password_char, input_placeholder_char;
    char erase_password_on_failure;
    int incorrect_credentials_message;
} config_t;

extern config_t config;

void load_config(void);

#endif  // CONFIG_H