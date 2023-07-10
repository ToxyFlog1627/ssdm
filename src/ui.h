#ifndef UI_H
#define UI_H

#include <ncurses.h>

#define I_USERNAME 0
#define I_PASSWORD 1

#define SHUTDOWN_KEY KEY_F(1)
#define REBOOT_KEY KEY_F(2)

void open_ui(void);
void focus_tty(void);
void prev_input(void);
void next_input(void);
void append_char(char ch);
void delete_char(void);
int get_ch(void);
const char *get_value(int input);
void show_message(const char *text);
void clear_input(int input);
void refresh_window(void);
void close_ui(void);

#endif  // UI_H