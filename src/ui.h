#ifndef UI_H
#define UI_H

#define I_USERNAME 0
#define I_PASSWORD 1

void show(void);
const char *get_value(int input);
void handle_input(int ch);
void close(void);

#endif  // UI_H