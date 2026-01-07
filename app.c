#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <strings.h>
#include "app.h"
#include "gui.h"
#include "db.h"
#include "screen_login.h"
#include "screen_intro.h"
#include "screen_chat.h"
#include "screen_dmesg.h"
#include "screen_cube.h"
#include "screen_demo.h"
#include "screen_debug.h"
#include "screen_poker.h"
#include "screen_admin.h"

void piper_speak(Client *c, const char *text, float speed) {
    if (c->xcmd_status != 1) return;

    // Sanitize text: Remove single quotes to prevent shell command breakage
    char safe_text[256];
    size_t j = 0;
    for (size_t i = 0; text[i] != '\0' && j < sizeof(safe_text) - 1; i++) {
        if (text[i] != '\'') {
            safe_text[j++] = text[i];
        }
    }
    safe_text[j] = '\0';

    char cmd[512];
    // Force dot as decimal separator manually to avoid locale issues (e.g. "1,3" vs "1.3")
    // Added +0.5 for rounding to fix float precision issues (1.299 -> 1.2)
    float final_speed = (speed > 0.0f) ? speed : CONF_TTS_GLOBAL_SPEED;
    int s_int = (int)final_speed;
    int s_dec = (int)((final_speed - s_int) * 10 + 0.5);
    if (s_dec >= 10) { s_int++; s_dec = 0; }

    snprintf(cmd, sizeof(cmd), "/home/simon/Dokumente/junkyard/bash-scripts/piper_wrap.sh %d.%d '%s' &", s_int, s_dec, safe_text);
   
    // Construct Telnet sequence manually to avoid string issues with 0xFF
    unsigned char buf[1024];
    size_t cmd_len = strlen(cmd);
    buf[0] = 0xFF; // IAC
    buf[1] = 0xFA; // SB
    buf[2] = 200;  // XCMD
    memcpy(buf + 3, cmd, cmd_len);
    buf[3 + cmd_len] = 0xFF; // IAC
    buf[4 + cmd_len] = 0xF0; // SE
    
    flush_client(c);
    send(c->fd, buf, 5 + cmd_len, 0);
}

void oabeep_play(Client *c, const char *tokens) {
    if (c->xcmd_status != 1) return;

    // Sanitize tokens: Remove single quotes to prevent shell command breakage
    char safe_tokens[256];
    size_t j = 0;
    for (size_t i = 0; tokens[i] != '\0' && j < sizeof(safe_tokens) - 1; i++) {
        if (tokens[i] != '\'') {
            safe_tokens[j++] = tokens[i];
        }
    }
    safe_tokens[j] = '\0';

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "/home/simon/Dokumente/junkyard/oabeep/oabeep %s &", safe_tokens);
   
    // Construct Telnet sequence manually
    unsigned char buf[1024];
    size_t cmd_len = strlen(cmd);
    buf[0] = 0xFF; // IAC
    buf[1] = 0xFA; // SB
    buf[2] = 200;  // XCMD
    memcpy(buf + 3, cmd, cmd_len);
    buf[3 + cmd_len] = 0xFF; // IAC
    buf[4 + cmd_len] = 0xF0; // SE
    
    flush_client(c);
    send(c->fd, buf, 5 + cmd_len, 0);
}

void show_extro(Client *c, const char *tts_msg, const char *txt_file) {
    send_client(c, ANSI_RESET); clear_screen(c);
    piper_speak(c, tts_msg, 0);
    FILE *fp = fopen(txt_file, "r");
    if (fp) {
        send_client(c, ANSI_RESET); char line[512]; int y = 3;
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = 0; write_text_aligned(c, line, y++, "CENTER");
        }
        fclose(fp);
    } else { set_cursor(c, 10, 5); send_client(c, TXT_BYE_FALLBACK); }
    send_client(c, "\r\n\r\n\r"); send_client(c, ANSI_SHOW_CURSOR); flush_client(c);
}

void show_extro_fail(Client *c) {
    send_client(c, ANSI_RESET); clear_screen(c);
    FILE *fp = fopen("extro_fail.txt", "r");
    if (fp) {
        send_client(c, ANSI_RESET); char line[512]; int y = 3;
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = 0; write_text_aligned(c, line, y++, "CENTER");
        }
        fclose(fp);
    } else { set_cursor(c, 10, 5); send_client(c, TXT_ACCESS_DENIED); }
    send_client(c, "\r");
    flush_client(c);
}

void add_to_blacklist(const char *email) {
    FILE *fp = fopen("blacklist.txt", "a");
    if (fp) {
        fprintf(fp, "%s\n", email);
        fclose(fp);
    }
}

int check_blacklist(const char *email) {
    FILE *fp = fopen("blacklist.txt", "r");
    if (!fp) return 0;
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        if (strcasecmp(line, email) == 0) {
            found = 1;
            break;
        }
    }
    fclose(fp);
    return found;
}

void render_client(Client *c) {
    switch(c->screen) {
        case SCREEN_LOGIN_USER:
        case SCREEN_LOGIN_PASS: render_login(c); break;
        case SCREEN_LOGIN_CODE: render_login(c); break;
        case SCREEN_INTRO: render_intro(c); break;
        case SCREEN_CARD: render_card(c); break;
        case SCREEN_BOX: render_box(c); break;
        case SCREEN_CUBE: render_cube(c); break;
        case SCREEN_KEY_DEBUG: render_debug(c); break;
        case SCREEN_DMESG: render_dmesg(c); break;
        case SCREEN_CHAT: render_chat(c); break;
        case SCREEN_POKER: render_poker(c); break;
        case SCREEN_ADMIN: render_admin(c); break;
        default: break;
    }
}

void handle_client_logic(Client *c, KeyCode key, char ch) {
    if (key == KEY_ESCAPE) { show_extro(c, TTS_BYE, "extro.txt"); c->active = 0; return; }
    if (key == KEY_F1) { c->screen = SCREEN_CHAT; c->needs_redraw = 1; return; }
    if (key == KEY_F2) { c->screen = SCREEN_CUBE; c->needs_redraw = 1; return; }
    if (key == KEY_F3) { c->screen = SCREEN_INTRO; c->needs_redraw = 1; return; }
    if (key == KEY_F4) { c->screen = SCREEN_POKER; c->needs_redraw = 1; return; }
    if (key == KEY_F5) { c->screen = SCREEN_KEY_DEBUG; c->needs_redraw = 1; return; }
    if (key == KEY_F12 && c->is_admin) { c->screen = SCREEN_DMESG; c->needs_redraw = 1; return; }
    if (key == KEY_F6 && c->is_admin) { c->screen = SCREEN_ADMIN; c->admin_focus = 0; c->admin_mode = 0; db_get_user_list(c); c->needs_redraw = 1; return; }

    if (key == KEY_CHAR) {
        int freq = 440 + rand() % 441;
        char buf[32];
        snprintf(buf, sizeof(buf), "%d:100", freq);
        oabeep_play(c, buf);
    }

    switch(c->screen) {
        case SCREEN_LOGIN_USER:
        case SCREEN_LOGIN_PASS:
        case SCREEN_LOGIN_CODE:
            handle_input_login(c, key, ch); break;
        case SCREEN_INTRO:
            handle_input_intro(c, key, ch); break;
        case SCREEN_CARD:
        case SCREEN_BOX: handle_input_demo(c, key, ch); break;
        case SCREEN_CUBE: handle_input_cube(c, key, ch); break;
        case SCREEN_KEY_DEBUG:
            handle_input_debug(c, key, ch); break;
        case SCREEN_CHAT:
            handle_input_chat(c, key, ch); break;
        case SCREEN_DMESG:
            handle_input_dmesg(c, key, ch); break;
        case SCREEN_POKER:
            handle_input_poker(c, key, ch); break;
        case SCREEN_ADMIN:
            handle_input_admin(c, key, ch); break;
        default: break;
    }
    c->needs_redraw = 1;
}