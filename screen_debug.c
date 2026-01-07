#include <stdio.h>
#include "screen_debug.h"
#include "gui.h"

void render_debug(Client *c) {
    clear_screen(c); draw_box(c, 20, 8, 40, 10, TXT_DEBUG_TITLE);
    set_cursor(c, 22, 10); send_client(c, TXT_DEBUG_LAST);
    set_cursor(c, 22, 12); set_color_fg(c, 0, 255, 0); send_client(c, c->debug_last_key);
    set_color_fg(c, 255, 255, 255); set_cursor(c, 22, 16); send_client(c, TXT_DEBUG_RETURN);
}

void handle_input_debug(Client *c, KeyCode key, char ch) {
    if (key == KEY_F5) return;
    if (key != KEY_CHAR) {
        c->debug_utf8_len = 0;
        switch(key) {
            case KEY_ENTER: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "ENTER"); break;
            case KEY_BACKSPACE: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "BACKSPACE"); break;
            case KEY_UP: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "UP"); break;
            case KEY_DOWN: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "DOWN"); break;
            case KEY_LEFT: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "LEFT"); break;
            case KEY_RIGHT: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "RIGHT"); break;
            case KEY_ESCAPE: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "ESCAPE"); break;
            case KEY_F1: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F1"); break;
            case KEY_F2: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F2"); break;
            case KEY_F3: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F3"); break;
            case KEY_F4: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F4"); break;
            case KEY_F5: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F5"); break;
            case KEY_F6: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F6"); break;
            case KEY_F7: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F7"); break;
            case KEY_F8: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F8"); break;
            case KEY_F9: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F9"); break;
            case KEY_F10: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F10"); break;
            case KEY_F11: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F11"); break;
            case KEY_F12: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "F12"); break;
            case KEY_TAB: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "TAB"); break;
            case KEY_INSERT: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "INSERT"); break;
            case KEY_DELETE: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "DELETE"); break;
            case KEY_HOME: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "HOME"); break;
            case KEY_END: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "END"); break;
            case KEY_PGUP: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "PAGE UP"); break;
            case KEY_PGDOWN: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "PAGE DOWN"); break;
            default: snprintf(c->debug_last_key, sizeof(c->debug_last_key), "UNKNOWN (%d)", key); break;
        }
    } else {
        unsigned char uc = (unsigned char)ch;
        if (uc < 128) { c->debug_utf8_len = 0; if (uc >= 32 && uc <= 126) snprintf(c->debug_last_key, sizeof(c->debug_last_key), "'%c' (ASCII %d)", uc, uc); else snprintf(c->debug_last_key, sizeof(c->debug_last_key), "CTRL %d", uc); }
        else { if ((uc & 0xC0) == 0xC0) { c->debug_utf8_len = 0; c->debug_utf8_buf[c->debug_utf8_len++] = ch; } else if ((uc & 0xC0) == 0x80) { if (c->debug_utf8_len > 0 && c->debug_utf8_len < 4) c->debug_utf8_buf[c->debug_utf8_len++] = ch; } c->debug_utf8_buf[c->debug_utf8_len] = '\0';
        unsigned char first = (unsigned char)c->debug_utf8_buf[0]; int expected = 0; if ((first & 0xE0) == 0xC0) expected = 2; else if ((first & 0xF0) == 0xE0) expected = 3; else if ((first & 0xF8) == 0xF0) expected = 4; if (c->debug_utf8_len == expected) { snprintf(c->debug_last_key, sizeof(c->debug_last_key), "UTF-8: %s", c->debug_utf8_buf); c->debug_utf8_len = 0; } }
    }
}