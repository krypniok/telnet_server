#ifndef GUI_H
#define GUI_H

#include "common.h"

void flush_client(Client *c);
void send_client(Client *c, const char *str);
void send_client_len(Client *c, const char *data, int len);
void set_cursor(Client *c, int x, int y);
void set_color_fg(Client *c, int r, int g, int b);
void set_color_bg(Client *c, int r, int g, int b);
void set_color_bg_hex(Client *c, const char *hex);
void clear_screen(Client *c);

int utf8_width(const char *s);
int draw_ascii_text(Client *c, int x, int y, const char *text, float phase);
void draw_box(Client *c, int x, int y, int w, int h, const char *title);
void draw_card(Client *c, int x, int y, const char *suit, const char *value);
void draw_list(Client *c);
void write_text_aligned(Client *c, const char *text, int y, const char *alignment);
void to_segment_digits(char *dest, size_t dest_size, const char *src);

#endif