#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/socket.h>
#include "gui.h"
#include "font.h"

void flush_client(Client *c) {
    if (c->out_len > 0) {
        ssize_t sent = send(c->fd, c->out_buf, c->out_len, 0);
        if (sent > 0) {
            c->upload += sent;
            if (sent < c->out_len) {
                // Partial send: move remaining data to front
                memmove(c->out_buf, c->out_buf + sent, c->out_len - sent);
                c->out_len -= sent;
            } else {
                c->out_len = 0;
            }
        } else if (sent < 0) {
            // Error handling to prevent infinite loops
            perror("send failed");
            c->active = 0; // Mark client as inactive
            c->out_len = 0; // Clear buffer
        }
    }
}

void send_client_len(Client *c, const char *data, int len) {
    if (c->out_len + len >= (int)sizeof(c->out_buf)) {
        flush_client(c);
    }
    if (c->out_len + len < (int)sizeof(c->out_buf)) {
        memcpy(c->out_buf + c->out_len, data, len);
        c->out_len += len;
    } else {
        // Buffer full even after flush, drop packet (prevent overflow)
        fprintf(stderr, "[ERROR] Output buffer full for client %d, dropping %d bytes\n", c->fd, len);
    }
}

void send_client(Client *c, const char *str) {
    send_client_len(c, str, strlen(str));
}

void set_cursor(Client *c, int x, int y) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[%d;%dH", y, x);
    send_client(c, buf);
}

void set_color_fg(Client *c, int r, int g, int b) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[38;2;%d;%d;%dm", r, g, b);
    send_client(c, buf);
}

void set_color_bg(Client *c, int r, int g, int b) {
    char buf[32];
    snprintf(buf, sizeof(buf), "\033[48;2;%d;%d;%dm", r, g, b);
    send_client(c, buf);
}

void set_color_bg_hex(Client *c, const char *hex) {
    unsigned int r, g, b;
    sscanf(hex, "%02x%02x%02x", &r, &g, &b);
    set_color_bg(c, r, g, b);
}

void clear_screen(Client *c) {
    send_client(c, ANSI_CLS);
}

int utf8_width(const char *s) {
    int count = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) count++;
        s++;
    }
    return count;
}

int draw_ascii_text(Client *c, int x, int y, const char *text, float phase) {
    int current_x = x;
    const char *ptr = text;
    int idx = 0;
    
    while (*ptr) {
        char ch = *ptr++;
        if (ch == ' ') {
            current_x += 4;
            idx++;
            continue;
        }
        
        const char *art = get_font_art(ch);
        if (!*art) { idx++; continue; }
        
        int offset = (int)(sin(phase + idx * 0.5f) * 2.0f);
        
        char art_copy[512];
        strncpy(art_copy, art, sizeof(art_copy));
        art_copy[sizeof(art_copy)-1] = '\0';
        
        char *line = strtok(art_copy, "\n");
        int max_w = 0;
        int row = 0;
        
        while (line) {
            int w = utf8_width(line);
            if (w > max_w) max_w = w;
            
            int cursor_x = current_x;
            int cursor_y = y + row + offset;
            char *p = line;
            while (*p) {
                if (*p == ' ') {
                    cursor_x++;
                    p++;
                } else {
                    char *start = p;
                    int chunk_width = 0;
                    while (*p && *p != ' ') {
                        if ((*p & 0xC0) != 0x80) chunk_width++;
                        p++;
                    }
                    set_cursor(c, cursor_x, cursor_y);
                    send_client_len(c, start, (int)(p - start));
                    cursor_x += chunk_width;
                }
            }
            
            line = strtok(NULL, "\n");
            row++;
        }
        current_x += max_w + 1;
        idx++;
    }
    return current_x - x;
}

void draw_box(Client *c, int x, int y, int w, int h, const char *title) {
    set_cursor(c, x, y);
    send_client(c, "┌");
    for(int i=0; i<w-2; i++) send_client(c, "─");
    send_client(c, "┐");
    
    for(int i=1; i<h-1; i++) {
        set_cursor(c, x, y+i);
        send_client(c, "│");
        for(int j=0; j<w-2; j++) send_client(c, " ");
        send_client(c, "│");
    }
    
    set_cursor(c, x, y+h-1);
    send_client(c, "└");
    for(int i=0; i<w-2; i++) send_client(c, "─");
    send_client(c, "┘");
    
    if (title && strlen(title) > 0) {
        set_cursor(c, x+2, y);
        send_client(c, title);
    }
}

void draw_card(Client *c, int x, int y, const char *suit, const char *value) {
    int is_red = (strcmp(suit, "hearts") == 0 || strcmp(suit, "diamonds") == 0);
    const char *symbol = "♠";
    if (strcmp(suit, "hearts") == 0) symbol = "♥";
    else if (strcmp(suit, "diamonds") == 0) symbol = "♦";
    else if (strcmp(suit, "clubs") == 0) symbol = "♣";
    
    set_color_bg(c, 255, 255, 255);
    set_color_fg(c, is_red ? 200 : 0, 0, 0);

    set_cursor(c, x, y);     send_client(c, "╭──────────╮");
    for(int i=1; i<7; i++) {
        set_cursor(c, x, y+i); send_client(c, "│          │");
    }
    set_cursor(c, x, y+7);   send_client(c, "╰──────────╯");

    set_cursor(c, x+2, y+1);
    char buf[32];
    snprintf(buf, sizeof(buf), "%-2s", value);
    send_client(c, buf);
    
    set_cursor(c, x+5, y+3);
    send_client(c, symbol);

    set_cursor(c, x+8, y+6);
    send_client(c, buf);
}

void draw_list(Client *c) {
    const char *items[] = {"NULL", "EINS", "ZWEI", "DREI"};
    int count = 4;
    int x = 3, y = 3;
    
    set_color_bg_hex(c, "630006");
    set_color_fg(c, 255, 255, 255);
    draw_box(c, x, y, 16, 8, TXT_BOX_LIST);
    
    for(int i=0; i<count; i++) {
        if (i == c->list_selected) set_color_bg_hex(c, "566733");
        else set_color_bg_hex(c, "333403");
        set_cursor(c, x+2, y+2+i);
        char buf[32];
        snprintf(buf, sizeof(buf), " %-10s ", items[i]);
        send_client(c, buf);
    }
}

void write_text_aligned(Client *c, const char *text, int y, const char *alignment) {
    int w = (c->width > 0) ? c->width : 80;
    int len = strlen(text);
    int x = 1;
    if (strcmp(alignment, "CENTER") == 0) x = (w - len) / 2 + 1;
    else if (strcmp(alignment, "RIGHT") == 0) x = w - len + 1;
    set_cursor(c, x, y);
    send_client(c, text);
}

void to_segment_digits(char *dest, size_t dest_size, const char *src) {
    size_t idx = 0;
    while (*src && idx < dest_size - 1) {
        if (*src >= '0' && *src <= '9') {
            if (idx + 4 >= dest_size) break;
            int digit = *src - '0';
            // 0x1FBF0 = F0 9F AF B0 in UTF-8
            dest[idx++] = 0xF0;
            dest[idx++] = 0x9F;
            dest[idx++] = 0xAF;
            dest[idx++] = 0xB0 + digit;
        } else {
            if (idx + 1 >= dest_size) break;
            dest[idx++] = *src;
        }
        src++;
    }
    dest[idx] = '\0';
}