#ifndef LOGO_ASCII_H_
#define LOGO_ASCII_H_

#include <stdio.h>

const unsigned char* logo_ascii[] = {
    " |                  '                      _|_      *                       '  .",
    "-+-         o            o           '  .   |           o  '      ':.           ",
    " |   /    o                                     .:'            .    '::._  .    ",
    "    /              .    \\      +     _..    _.::'                     '._)    ' ",
    "   *         .           \\         '`-. `. (_.'         _|_   ~~+ +~~      '    ",
    "                  ':.     *            \\  \\     .:'~~+   |                      ",
    "       o   +        '::._   .          |  | _.::'                      '   |    ",
    "                 +    '._)             /  /(_.'                 ':.      - o -  ",
    "         o   '                     _.-`_.`         *              '::._    |   o",
    "   .                           +~~  '''                             '._)        ",
    "         |               +~~               *              .      .:'  +~~       ",
    "       --o--                  .         .        .           _.::'        '     ",
    " .       |                                    o        |    (_.'               *",
    "             ~~+               '                 .   - o -                   '  ",
    "                                  +        |           |          *     .      '",
    " o                 ~~+              ~~+  --o--       ':.                  .     ",
    "                         ':.     '         |        /  '::._   *   ' '          ",
    "         ~~+   +~~         '::._                   /     '._)                 o ",
    ".                     .      '._)       . '       *                             ",
    "  '         ~~+                                            '      *            ."
};

static inline void print_ascii_banner(void)
{
    size_t i;

    for (i = 0U; i < (sizeof(logo_ascii) / sizeof(logo_ascii[0])); ++i) {
        printf("%s\r\n", logo_ascii[i]);
    }
}

#endif /* LOGO_ASCII_H_ */
