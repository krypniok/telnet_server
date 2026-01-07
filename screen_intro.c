#include <stdio.h>
#include <string.h>
#include <time.h>
#include "screen_intro.h"
#include "gui.h"

void render_intro(Client *c) {
    set_color_bg(c, 45, 49, 3);
    clear_screen(c);
    set_color_fg(c, 255, 255, 255);
    set_cursor(c, 2, 1); send_client(c, TXT_TITLE_MAIN);
    const char *right_txt = TXT_SUBTITLE;
    int rx = (c->width > 0) ? c->width - strlen(right_txt) + 1 : 60;
    set_cursor(c, rx, 1); send_client(c, right_txt);
    const char *center_txt = TXT_CENTER_MSG;
    int cx = (c->width > 0) ? (c->width - strlen(center_txt)) / 2 : 30;
    int cy = (c->height > 0) ? c->height / 2 : 12;
    set_cursor(c, cx, cy); send_client(c, center_txt);
    set_cursor(c, 10, 16); set_color_fg(c, 192, 144, 128); send_client(c, c->input_buffer);
    set_cursor(c, 10, 18); send_client(c, c->msg);
    time_t now = time(NULL); struct tm *t = localtime(&now);
    char time_buf[16]; snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    set_cursor(c, 10, 10); set_color_fg(c, 0x74, 0x7f, 0x5d); send_client(c, time_buf);
    char traffic_buf[32]; set_color_fg(c, 192, 144, 128);
    set_cursor(c, 70, 10); snprintf(traffic_buf, sizeof(traffic_buf), "%ld", c->upload); send_client(c, traffic_buf);
    set_cursor(c, 70, 12); snprintf(traffic_buf, sizeof(traffic_buf), "%ld", c->download); send_client(c, traffic_buf);
    set_cursor(c, 70, 14); set_color_fg(c, c->xcmd_status == 1 ? 0 : 255, c->xcmd_status == 1 ? 255 : 0, 0);
    send_client(c, c->xcmd_status == 1 ? TXT_XCMD_ON : TXT_XCMD_OFF);
    draw_list(c);
}

void handle_input_intro(Client *c, KeyCode key, char ch) {
    (void)ch;
    if (key == KEY_DOWN) c->list_selected = (c->list_selected + 1) % 4;
    else if (key == KEY_UP) c->list_selected = (c->list_selected - 1 + 4) % 4;
    else if (key == KEY_ENTER) c->screen = SCREEN_CARD;
}