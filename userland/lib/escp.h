/*
 *  .OOOOOO.   OOOO                                .    O8O              
 *  D8P'  `Y8B  `888                              .O8    `"'              
 * 888           888 .OO.    .OOOO.    .OOOOO.  .O888OO OOOO  OOOO    OOO 
 * 888           888P"Y88B  `P  )88B  D88' `88B   888   `888   `88B..8P'  
 * 888           888   888   .OP"888  888   888   888    888     Y888'    
 * `88B    OOO   888   888  D8(  888  888   888   888 .  888   .O8"'88B   
 *  `Y8BOOD8P'  O888O O888O `Y888""8O `Y8BOD8P'   "888" O888O O88'   888O 
 * 
 *  Chaotix is a UNIX-like operating system that consists of a kernel written in C and
 *  i?86 assembly, and userland binaries written in C.
 *     
 *  Copyright (c) 2023 Nexuss
 *  Copyright (c) 2022 mosm
 *  Copyright (c) 2006-2018 Frans Kaashoek, Robert Morris, Russ Cox, Massachusetts Institute of Technology
 *
 *  This file may or may not contain code from https://github.com/mosmeh/yagura, and/or
 *  https://github.com/mit-pdos/xv6-public. Both projects have the same license as this
 *  project, and the license can be seen below:
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 */

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