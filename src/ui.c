#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include "utils.h"

#define MIN_WIDTH 50
#define MAX_WIDTH 100
#define MIN_HEIGHT 8
#define MAX_HEIGHT 16

#define H_PAD 2

void show(void) {
    int w_width, w_height, i_x = H_PAD, i_y, t_x;
    const char *text;

    initscr();
    refresh();

    keypad(stdscr, TRUE);

    w_width = clamp(COLS / 2, MIN_WIDTH, MAX_WIDTH);
    w_height = clamp(LINES / 2, MIN_HEIGHT, MAX_HEIGHT);

    WINDOW *win = newwin(w_height, w_width, (LINES - w_height) / 2, (COLS - w_width) / 2);
    box(win, NULL, NULL);

    text = "password:";
    t_x = strlen(text) + H_PAD + 1;
    i_y = w_height - 2;
    mvwprintw(win, i_y, i_x, text);
    mvwhline(win, i_y, t_x, '_', w_width - t_x - H_PAD);

    text = "login:";
    i_y = w_height - 4;
    mvwprintw(win, i_y, i_x, text);
    mvwhline(win, i_y, t_x, '_', w_width - t_x - H_PAD);

    text = "ssdm";
    mvwprintw(win, 1, (w_width - strlen(text)) / 2, text);

    wrefresh(win);
}

void close(void) { endwin(); }