#ifndef FONT_H
#define FONT_H

#include <stddef.h>
#include <ctype.h>

typedef struct {
    char key;
    const char *art;
} FontGlyph;

/* 
 * ASCII Font Definitions
 * Format: Key, followed by the multi-line string.
 */
static const FontGlyph font_glyphs[] = {
    { 'a', 
        "\n"
        "▗▞▀▜▌\n"
        "▝▚▄▟▌\n"
        "     \n"
        "     \n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'b',
        "\n"
        "▗▖   \n"
        "▐▌   \n"
        "▐▛▀▚▖\n"
        "▐▙▄▞▘\n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'c',
        "\n"
        "▗▞▀▘\n"
        "▝▚▄▖\n"
        "    \n"
        "    \n"
        "    \n"
        "    \n"
        "    \n"
    },
    { 'd',
        "\n"
        "   ▐▌\n"
        "   ▐▌\n"
        "▗▞▀▜▌\n"
        "▝▚▄▟▌\n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'e',
        "\n"
        "▗▞▀▚▖\n"
        "▐▛▀▀▘\n"
        "▝▚▄▄▖\n"
        "     \n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'f',
        "\n"
        "▗▞▀▀▘\n"
        "▐▌   \n"
        "▐▛▀▘ \n"
        "▐▌   \n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'g',
        "\n"
        "     \n"
        "     \n"
        "     \n"
        " ▗▄▖ \n"
        "▐▌ ▐▌\n"
        " ▝▀▜▌\n"
        "▐▙▄▞▘\n"
        "                 \n"
    },
    { 'h',
        "\n"
        "▐▌   \n"
        "▐▌   \n"
        "▐▛▀▚▖\n"
        "▐▌ ▐▌\n"
        "     \n"
        "     \n"
        "     \n"
        "           \n"
    },
    { 'i',
        "\n"
        "▄ \n"
        "▄ \n"
        "█ \n"
        "█ \n"
        "  \n"
        "  \n"
        "  \n"
    },
    { 'j',
        "\n"
        "   ▗▖\n"
        "   ▗▖\n"
        "▄  ▐▌\n"
        "▀▄▄▞▘\n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'k',
        "\n"
        "█  ▄ \n"
        "█▄▀  \n"
        "█ ▀▄ \n"
        "█  █ \n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'l',
        "\n"
        "█ \n"
        "█ \n"
        "█ \n"
        "█ \n"
        "  \n"
        "  \n"
        "  \n"
    },
    { 'm',
        "\n"
        "▄▄▄▄  \n"
        "█ █ █ \n"
        "█   █ \n"
        "      \n"
        "      \n"
        "      \n"
        "      \n"
    },
    { 'n',
        "\n"
        "▄▄▄▄  \n"
        "█   █ \n"
        "█   █ \n"
        "      \n"
        "      \n"
        "      \n"
        "      \n"
    },
    { 'o',
        "\n"
        " ▄▄▄  \n"
        "█   █ \n"
        "▀▄▄▄▀ \n"
        "      \n"
        "      \n"
        "      \n"
        "      \n"
    },
    { 'p',
        "\n"
        "▄▄▄▄  \n"
        "█   █ \n"
        "█▄▄▄▀ \n"
        "█     \n"
        "▀     \n"
        "      \n"
        "      \n"
    },
    { 'q',
        "\n"
        " ▄▄▄▄ \n"
        "█   █ \n"
        "▀▄▄▄█ \n"
        "    █ \n"
        "    ▀ \n"
        "      \n"
        "      \n"
    },
    { 'r',
        "\n"
        " ▄▄▄ \n"
        "█    \n"
        "█    \n"
        "     \n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 's',
        "\n"
        " ▄▄▄ \n"
        "▀▄▄  \n"
        "▄▄▄▀ \n"
        "     \n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 't',
        "\n"
        "   ■  \n"
        "▗▄▟▙▄▖\n"
        "  ▐▌  \n"
        "  ▐▌  \n"
        "  ▐▌  \n"
        "      \n"
        "      \n"
    },
    { 'u',
        "\n"
        "█  ▐▌\n"
        "▀▄▄▞▘\n"
        "     \n"
        "     \n"
        "     \n"
        "     \n"
        "     \n"
    },
    { 'v',
        "\n"
        "▄   ▄ \n"
        "█   █ \n"
        " ▀▄▀  \n"
        "      \n"
        "      \n"
        "      \n"
        "      \n"
    },
    { 'w',
        "\n"
        "▄   ▄ \n"
        "█ ▄ █ \n"
        "█▄█▄█ \n"
        "      \n"
        "      \n"
        "      \n"
        "      \n"
    },
    { 'x',
        "\n"
        "▄   ▄ \n"
        " ▀▄▀  \n"
        "▄▀ ▀▄ \n"
        "      \n"
        "      \n"
        "      \n"
        "      \n"
    },
    { 'y',
        "\n"
        "▄   ▄ \n"
        "█   █ \n"
        " ▀▀▀█ \n"
        "▄   █ \n"
        " ▀▀▀  \n"
        "      \n"
        "      \n"
    },
    { 'z',
        "\n"
        "▄▄▄▄▄ \n"
        " ▄▄▄▀ \n"
        "█▄▄▄▄ \n"
        "      \n"
        "      \n"
        "      \n"
        "      \n"
    },
    { ' ',
        "\n"
        "        \n"
        "        \n"
        "        \n"
        "        \n"
        "        \n"
        "        \n"
        "        \n"
    },
    { 0, NULL }
};

static const char *get_font_art(char c) {
    char lower = tolower(c);
    for (size_t i = 0; font_glyphs[i].art; i++) {
        if (font_glyphs[i].key == lower) {
            return font_glyphs[i].art;
        }
    }
    return "";
}

#endif