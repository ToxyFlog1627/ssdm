#include "ui.h"
#include <assert.h>
#include <locale.h>
#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include "config.h"
#include "utils.h"

#define MIN_WIDTH 40
#define MAX_WIDTH 60
#define HEIGHT 10
#define H_PAD 3

#define TITLE "ssdm"

#define SHUTDOWN_KEY KEY_F(1)
#define SHUTDOWN_TEXT "F1 shutdown"
#define REBOOT_KEY KEY_F(2)
#define REBOOT_TEXT "F2 reboot"

typedef struct INPUT {
    int y, x, tx, width, i;
    const char *text;
    char *value;
    char hide_input;
} INPUT;

#define MAX_INPUT_LENGTH 512
#define INPUTS 2

#define UPDATE_CARET_POSITION() wmove(win, inputs[selected_input].y, inputs[selected_input].tx + inputs[selected_input].i + 1)

INPUT inputs[INPUTS];
int selected_input = 0;
char is_message_shown = 0;
WINDOW *win;

INPUT new_input(const char *text, int y, int x, int total_width, int tx, char hide_input) {
    assert(text != NULL && *text != '\0' && y > 0 && x > 0 && (hide_input == 0 || hide_input == 1));

    int text_length = strlen(text) + 1;
    assert(total_width > text_length);

    char *value = (char *) malloc(sizeof(char) * MAX_INPUT_LENGTH);
    if (value == NULL) {
        syslog(LOG_EMERG, "Bad malloc of input");
        exit(EXIT_FAILURE);
    }

    memset(value, '\0', MAX_INPUT_LENGTH);
    if (tx == 0) tx = x + text_length;
    INPUT input = {y, x, tx, total_width - text_length - 2, -1, text, value, hide_input};

    mvwprintw(win, input.y, input.x, input.text);

    return input;
}

void open_ui(void) {
    initscr();
    keypad(stdscr, TRUE);
    noecho();

    mvprintw(0, 0, SHUTDOWN_TEXT);
    mvprintw(0, strlen(SHUTDOWN_TEXT) + 2, REBOOT_TEXT);

    refresh();

    int w_width = clamp(COLS / 2, MIN_WIDTH, MAX_WIDTH), w_height = HEIGHT;
    win = newwin(w_height, w_width, (LINES - w_height) / 2, (COLS - w_width) / 2);
    box(win, 0, 0);

    mvwprintw(win, 1, (w_width - strlen(TITLE)) / 2, TITLE);

    inputs[1] = new_input("password", w_height - 3, H_PAD, w_width - H_PAD, 0, 1);
    inputs[0] = new_input("login", w_height - 5, H_PAD, w_width - H_PAD, inputs[1].tx, 0);

    wmove(win, inputs[selected_input].y, inputs[selected_input].tx);
    wrefresh(win);
}

void append_char(char ch) {
    assert(ch >= '!' && ch <= '~');

    INPUT *input = &inputs[selected_input];
    if (input->i + 2 == MAX_INPUT_LENGTH) return;
    input->value[++input->i] = ch;

    if (input->hide_input) ch = config.password_char;

    if (input->i < input->width) mvwaddch(win, input->y, input->tx + input->i, ch);
    else mvwprintw(win, input->y, input->tx, input->value + input->i - input->width + 1);
}

void delete_char(void) {
    INPUT *input = &inputs[selected_input];
    if (input->i == -1) return;
    input->value[input->i] = '\0';

    if (input->i < input->width) mvwaddch(win, input->y, input->tx + input->i, config.input_placeholder_char);
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
        case SHUTDOWN_KEY:
            int pid = fork();
            if (pid == 0) system("poweroff");
            if (pid == -1) syslog(LOG_CRIT, "Unable to shutdown");
            break;
        case REBOOT_KEY:
            int pid = fork();
            if (pid == 0) system("reboot");
            if (pid == -1) syslog(LOG_CRIT, "Unable to reboot");
            break;
        default:
            if (ch >= '!' && ch <= '~') append_char(ch);
            break;
    }

    if (is_message_shown) {
        is_message_shown = 0;
        mvwhline(win, 3, 1, ' ', getmaxx(win) - 2);
    }

    UPDATE_CARET_POSITION();
    wrefresh(win);
}

void show_message(const char *text) {
    assert(text != NULL && *text != '\0');

    is_message_shown = 1;
    int width = getmaxx(win), text_length = strlen(text);
    if (text_length + H_PAD >= width) return;
    mvwprintw(win, 3, (width - text_length) / 2, text);

    UPDATE_CARET_POSITION();
    wrefresh(win);
}

void close_ui(void) {
    for (int i = 0; i < INPUTS; i++) free(inputs[i].value);
    endwin();
}