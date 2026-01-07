#include <stdio.h>
#include <string.h>
#include "screen_dmesg.h"
#include "gui.h"

void render_dmesg(Client *c) {
    set_color_bg(c, 0, 0, 0);
    clear_screen(c);
    int h = (c->height > 0) ? c->height : 24;
    int w = (c->width > 0) ? c->width : 80;
    
    set_cursor(c, 1, 1); set_color_bg_hex(c, "444444"); set_color_fg(c, 255, 255, 255);
    char header[128]; snprintf(header, sizeof(header), TXT_DMESG_TITLE, w, h);
    send_client(c, header);
    
    set_color_bg(c, 0, 0, 0); set_color_fg(c, 180, 180, 180);
    char cmd[64]; int lines = h - 2; if (lines < 1) lines = 1;
    snprintf(cmd, sizeof(cmd), "dmesg | tail -n %d", lines);
    
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char line[512]; int y = 2;
        while (fgets(line, sizeof(line), fp)) {
            if (y >= h) break;
            line[strcspn(line, "\r\n")] = 0;
            if ((int)strlen(line) > w) line[w] = 0;
            set_cursor(c, 1, y++);
            send_client(c, line);
        }
        pclose(fp);
    } else {
        set_cursor(c, 1, 2); set_color_fg(c, 255, 0, 0); send_client(c, TXT_DMESG_ERR);
    }
    
    set_cursor(c, 1, h); set_color_bg_hex(c, "444444"); set_color_fg(c, 255, 255, 0);
    send_client(c, TXT_DMESG_HINT);
}

void handle_input_dmesg(Client *c, KeyCode key, char ch) {
    (void)c; (void)key; (void)ch;
    // No specific input handling for dmesg yet, just refresh via render loop or F12 global
}