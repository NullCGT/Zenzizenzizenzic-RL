#ifndef COLOR_H
#define COLOR_H

struct w_color {
    const char *str;
    unsigned char cnum;
    unsigned char r, g, b;
};

enum colornum {
    BLACK,
    RED,
    GREEN,
    YELLOW,
    BLUE,
    MAGENTA,
    CYAN,
    WHITE,
    /* Bright Colors */
    DARK_GRAY, // BRIGHT_BLACK,
    BRIGHT_RED,
    BRIGHT_GREEN,
    BRIGHT_YELLOW,
    BRIGHT_BLUE,
    BRIGHT_MAGENTA,
    BRIGHT_CYAN,
    BRIGHT_WHITE
};

#define BRIGHT_COLOR DARK_GRAY
#define MAX_COLOR BRIGHT_WHITE + 1

extern struct w_color w_colors[MAX_COLOR];

#endif