#include "screen_poker.h"
#include "gui.h"

void render_poker(Client *c) {
    set_color_bg_hex(c, "003300"); // Dunkelgrüner Filztisch
    clear_screen(c);
    
    set_cursor(c, 2, 2);
    set_color_fg(c, 255, 255, 255);
    send_client(c, TXT_POKER_TITLE);

    // Beispielhand
    draw_card(c, 5, 5, "spades", "A");
    draw_card(c, 20, 5, "hearts", "10");
    draw_card(c, 35, 5, "diamonds", "K");
    draw_card(c, 50, 5, "clubs", "7");
    draw_card(c, 65, 5, "hearts", "Q");
    
    set_cursor(c, 5, 15);
    set_color_bg_hex(c, "003300");
    set_color_fg(c, 150, 150, 150);
    send_client(c, TXT_POKER_HINT);
}

void handle_input_poker(Client *c, KeyCode key, char ch) {
    (void)c; (void)key; (void)ch;
}