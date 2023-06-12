#include "ui.h"
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define MIN_WIDTH 40
#define MAX_WIDTH 60
#define MIN_HEIGHT 10
#define MAX_HEIGHT 20
#define H_PAD 3

#define PASSWORD_CHAR '*'

#define MAX_INPUT_LENGTH 512

typedef struct INPUT {
    int y, x, tx, width, i;
    const char *text;
    char *value;
    char hide_input;
} INPUT;

#define INPUTS 2

INPUT inputs[INPUTS];
int selected_input = 0;
char is_message_shown = 0;
WINDOW *win;

INPUT new_input(const char *text, int y, int x, int total_width, char hide_input) {
    size_t text_length = strlen(text) + 1;

    char *value = (char *) malloc(sizeof(char) * MAX_INPUT_LENGTH);
    memset(value, '\0', MAX_INPUT_LENGTH);
    INPUT input = {y, x, input.x + text_length, total_width - text_length - 2, -1, text, value, hide_input};

    mvwprintw(win, input.y, input.x, input.text);

    return input;
}

void open_ui(void) {
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    refresh();

    int w_width = clamp(COLS / 2, MIN_WIDTH, MAX_WIDTH), w_height = clamp(LINES / 2, MIN_HEIGHT, MAX_HEIGHT);
    win = newwin(w_height, w_width, (LINES - w_height) / 2, (COLS - w_width) / 2);
    box(win, 0, 0);

    const char *title = "ssdm";
    mvwprintw(win, 1, (w_width - strlen(title)) / 2, title);

    inputs[0] = new_input("login", w_height - 5, H_PAD, w_width - H_PAD, 0);
    inputs[1] = new_input("password", w_height - 3, H_PAD, w_width - H_PAD, 1);
    inputs[0].tx = inputs[1].tx;

    wrefresh(win);
}

void append_char(char ch) {
    INPUT *input = &inputs[selected_input];
    if (input->i + 2 == MAX_INPUT_LENGTH) return;
    input->value[++input->i] = ch;

    if (input->hide_input) ch = PASSWORD_CHAR;

    if (input->i < input->width) mvwaddch(win, input->y, input->tx + input->i, ch);
    else mvwprintw(win, input->y, input->tx, input->value + input->i - input->width + 1);
}

void delete_char(void) {
    INPUT *input = &inputs[selected_input];
    if (input->i == -1) return;
    input->value[input->i] = '\0';

    if (input->i < input->width) mvwaddch(win, input->y, input->tx + input->i, '_');
    else mvwprintw(win, input->y, input->tx, input->value + input->i - input->width);
    input->i--;
}

const char *get_value(int input) { return input < INPUTS ? inputs[input].value : NULL; }

void handle_input(int ch) {
    switch (ch) {
        case KEY_BTAB:
        case KEY_UP:
            selected_input = selected_input > 0 ? (selected_input - 1) : (INPUTS - 1);
            break;
        case '\t':
        case KEY_DOWN:
            selected_input = (selected_input + 1) % INPUTS;
            break;
        case KEY_BACKSPACE:
        case 127:
        case '\b':
            delete_char();
            break;
        default:
            if (ch >= '!' && ch <= '~') append_char(ch);
            break;
    }

    if (is_message_shown) {
        is_message_shown = 0;
        mvwhline(win, 3, 1, ' ', getmaxx(win) - 2);
    }

    wrefresh(win);
}

void show_message(const char *text) {
    is_message_shown = 1;
    int width = getmaxx(win), text_length = strlen(text);
    if (text_length + H_PAD >= width) return;
    mvwprintw(win, 3, (width - text_length) / 2, text);
    wrefresh(win);
}

void close_ui(void) {
    free(inputs[0].value);
    free(inputs[1].value);
    endwin();
}