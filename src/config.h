#ifndef CONFIG_H
#define CONFIG_H

typedef struct config_t {
    char password_char, input_placeholder_char;
} config_t;

extern config_t config;

void load_config(void);

#endif  // CONFIG_H