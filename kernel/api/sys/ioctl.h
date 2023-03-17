#pragma once

enum { TIOCGPGRP, TIOCSPGRP, TIOCGWINSZ };

struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};
