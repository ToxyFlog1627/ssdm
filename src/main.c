#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include "ui.h"

#define MAX_INPUT_LENGTH 256

int main(void) {
    show();

    // getnstr();
    getch();

    close();
    return EXIT_SUCCESS;
}

// TODO: error handling for all ncurses method + how to display it? Just log?