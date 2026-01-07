#include "screen_demo.h"
#include "gui.h"

void render_card(Client *c) {
    draw_card(c, 10, 10, "spades", "Q"); set_cursor(c, 1, 20); send_client(c, TXT_DEMO_DISCONNECT);
}

void render_box(Client *c) {
    clear_screen(c); draw_box(c, 10, 10, 20, 10, TXT_BOX_HEARTS_10); set_cursor(c, 1, 20); send_client(c, TXT_DEMO_RETURN);
}

void handle_input_demo(Client *c, KeyCode key, char ch) {
    (void)ch;
    if (c->screen == SCREEN_CARD) {
        if (key == KEY_ENTER) c->screen = SCREEN_BOX;
    } else if (c->screen == SCREEN_BOX) {
        if (key == KEY_ENTER) c->screen = SCREEN_CUBE;
    }
}