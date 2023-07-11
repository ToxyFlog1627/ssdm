#ifndef UI_H
#define UI_H

#include <ncurses.h>

#define I_USERNAME 0
#define I_PASSWORD 1

#define SHUTDOWN_KEY KEY_F(1)
#define REBOOT_KEY KEY_F(2)

void append_char(char ch);
void clear_input(int input);
void close_ui(void);
void delete_char(void);
void focus_tty(void);
int get_ch(void);
const char *get_value(int input);
void next_input(void);
void open_ui(void);
void prev_input(void);
void refresh_window(void);
void show_message(const char *text);

#endif  // UI_H