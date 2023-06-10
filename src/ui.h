#ifndef UI_H
#define UI_H

#define I_USERNAME 0
#define I_PASSWORD 1

void open_ui(void);
const char *get_value(int input);
void handle_input(int ch);
void show_message(const char *text);
void close_ui(void);

#endif  // UI_H