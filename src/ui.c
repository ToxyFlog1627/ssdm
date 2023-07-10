#define _POSIX_C_SOURCE 200809L
#include "ui.h"
#include <assert.h>
#include <linux/vt.h>
#include <locale.h>
#include <ncurses.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <unistd.h>
#include "config.h"
#include "store.h"

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define clamp(v, l, u) min(max(v, l), u)

#define MIN_WIDTH 40
#define MAX_WIDTH 60
#define HEIGHT 10
#define H_PAD 3

#define MAX_INPUT_LENGTH 512
#define INPUTS 2

#define TITLE "ssdm"
#define SHUTDOWN_TEXT "F1 shutdown"
#define REBOOT_TEXT "F2 reboot"

#define UPDATE_CARET_POSITION() wmove(win, inputs[selected_input].y, inputs[selected_input].tx + inputs[selected_input].i + 1)

#define CLEAR_INPUT(input)                                                                  \
    {                                                                                       \
        memset((input).value, '\0', MAX_INPUT_LENGTH + 1);                                  \
        mvwhline(win, (input).y, (input).tx, config.input_placeholder_char, (input).width); \
    }

typedef struct INPUT {
    int y, x, tx, width, i;
    const char *text;
    char *value;
    char hide_input;
} INPUT;

INPUT inputs[INPUTS];
int selected_input = 0;
char is_ui_opened = 0;
WINDOW *win;

INPUT new_input(const char *text, int y, int x, int total_width, int tx, char hide_input) {
    assert(text != NULL && text[0] != '\0' && y > 0 && x > 0 && (hide_input == 0 || hide_input == 1));

    int text_length = strlen(text) + 1;
    assert(total_width > text_length);

    char *value = (char *) malloc(sizeof(char) * (MAX_INPUT_LENGTH + 1));
    if (value == NULL) {
        syslog(LOG_EMERG, "Bad malloc of input");
        exit(EXIT_FAILURE);
    }

    if (tx == 0) tx = x + text_length;
    INPUT input = {y, x, tx, total_width - tx, -1, text, value, hide_input};

    CLEAR_INPUT(input);
    mvwprintw(win, input.y, input.x, input.text);

    return input;
}

void set_value(int input_index, char *value) {
    assert(input_index >= 0 && input_index < INPUTS);

    size_t length = strlen(value);
    if (length >= MAX_INPUT_LENGTH) {
        syslog(LOG_ALERT, "Unable to set value that is longer than max input length");
        return;
    }

    INPUT *input = &inputs[input_index];
    input->i = length - 1;

    CLEAR_INPUT(*input);
    strcpy(input->value, value);
    mvwprintw(win, input->y, input->tx, input->value);
}

void load_username(void) {
    void *value = load("username");
    if (value == NULL) return;

    selected_input = I_PASSWORD;
    set_value(I_USERNAME, (char *) value);
    free(value);
}

void open_ui(void) {
    if (is_ui_opened) return;

    initscr();
    setlocale(LC_ALL, "C");
    keypad(stdscr, TRUE);
    noecho();

    clear();
    mvprintw(0, 0, SHUTDOWN_TEXT);
    mvprintw(0, strlen(SHUTDOWN_TEXT) + 2, REBOOT_TEXT);
    refresh();

    int w_width = clamp(COLS / 2, MIN_WIDTH, MAX_WIDTH), w_height = HEIGHT;
    win = newwin(w_height, w_width, (LINES - w_height) / 2, (COLS - w_width) / 2);
    box(win, 0, 0);

    mvwprintw(win, 1, (w_width - strlen(TITLE)) / 2, TITLE);

    inputs[1] = new_input("password", w_height - 3, H_PAD, w_width - H_PAD, 0, 1);
    inputs[0] = new_input("login", w_height - 5, H_PAD, w_width - H_PAD, inputs[1].tx, 0);

    if (config.save_login == 1) load_username();

    wmove(win, inputs[selected_input].y, inputs[selected_input].tx);
    wrefresh(win);

    is_ui_opened = 1;
}

void focus_tty(void) {
    FILE *console = fopen("/dev/console", "w");
    if (console == NULL) {
        syslog(LOG_CRIT, "Unable to open /dev/console!");
        return;
    }

    int fd = fileno(console);
    int tty = strtol(ttyname(STDIN_FILENO) + strlen("/dev/tty"), NULL, 10);

    if (ioctl(fd, VT_ACTIVATE, tty) == -1) syslog(LOG_CRIT, "Unable to focus tty!");
    if (ioctl(fd, VT_WAITACTIVE, tty) == -1) syslog(LOG_CRIT, "Unable to wait for tty to get focused!");

    fclose(console);
}

void next_input(void) { selected_input = (selected_input + 1) % INPUTS; }

void prev_input(void) { selected_input = (selected_input > 0 ? selected_input : INPUTS) - 1; }

void append_char(char ch) {
    if (ch < '!' || ch > '~') return;

    INPUT *input = &inputs[selected_input];
    if (input->i + 1 == MAX_INPUT_LENGTH) return;
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

int get_ch(void) { return getch(); }

const char *get_value(int input) {
    assert(input >= 0 && input < INPUTS);
    return inputs[input].value;
}

void hide_message(int sig) {
    (void) sig;
    mvwhline(win, 3, 1, ' ', getmaxx(win) - 2);
    refresh_window();
}

void show_message(const char *text) {
    assert(text != NULL && text[0] != '\0');

    int width = getmaxx(win), text_length = strlen(text);
    if (text_length + H_PAD >= width) return;

    mvwprintw(win, 3, (width - text_length) / 2, text);

    struct sigaction action;
    action.sa_handler = hide_message;
    action.sa_flags = SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        syslog(LOG_CRIT, "Unable to set SIGALRM handler");
        return;
    }

    alarm(config.error_message_duration_seconds);
}

void clear_input(int input) {
    assert(input >= 0 && input < INPUTS);

    CLEAR_INPUT(inputs[input]);
    inputs[input].i = -1;
}

void refresh_window(void) {
    UPDATE_CARET_POSITION();
    wrefresh(win);
}

void close_ui(void) {
    if (!is_ui_opened) return;
    is_ui_opened = 0;

    for (int i = 0; i < INPUTS; i++) free(inputs[i].value);

    endwin();
    win = NULL;

    alarm(0);
}