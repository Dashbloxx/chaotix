#pragma once

/*
 *  This header contains ANSI escape codes useful for having color in our text. If the defines start with `F_`, then they are colors that apply for the foreground.
 *  If the defines start with `B_`, then they apply for the background. Lines 8-13 don't apply to either background or foreground text.
 */

#define BOLD            "\x1b[1m"
#define ITALIC          "\x1b[3m"
#define UNDERLINE       "\x1b[4m"
#define STRIKETHROUGH   "\x1b[9m"

#define RESET           "\x1b[0m"

#define F_DEFAULT_COLOR "\x1b[39m"
#define F_BLACK         "\x1b[30m"
#define F_RED           "\x1b[31m"
#define F_GREEN         "\x1b[32m"
#define F_YELLOW        "\x1b[33m"
#define F_BLUE          "\x1b[34m"
#define F_MAGENTA       "\x1b[35m"

#define B_DEFAULT_COLOR "\x1b[49m"
#define B_BLACK         "\x1b[40m"
#define B_RED           "\x1b[41m"
#define B_GREEN         "\x1b[42m"
#define B_YELLOW        "\x1b[43m"
#define B_BLUE          "\x1b[44m"
#define B_MAGENTA       "\x1b[45m"