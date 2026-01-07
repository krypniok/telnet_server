#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "screen_login.h"
#include "gui.h"
#include "db.h"
#include "app.h"
#include "email.h"

static void login_success(Client *c) {
    c->login_time = time(NULL);
    char buf[256];
    if (c->is_new_user) {
        snprintf(buf, sizeof(buf), TTS_WELCOME_NEW);
    } else {
        snprintf(buf, sizeof(buf), TTS_WELCOME, c->username);
    }
    piper_speak(c, buf, 0);
    c->screen = SCREEN_CHAT;
}

void render_login(Client *c) {
    int w = (c->width > 0) ? c->width : 80; int h = (c->height > 0) ? c->height : 24;
    int bw = 40, bh = 5; int x = (w - bw) / 2; if (x < 1) x = 1; int y = (h - bh) / 2; if (y < 1) y = 1;
    set_color_bg_hex(c, "050510"); clear_screen(c);

    // Time & Countdown Logic
    setenv("TZ", "Europe/Berlin", 1); tzset();
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[32], count_str[32];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    
    int elapsed = (int)difftime(now, c->connect_time);
    int remaining = CONF_TIMEOUT_LOGIN - elapsed;
    if (remaining < 0) remaining = 0;
    snprintf(count_str, sizeof(count_str), "00:%02d", remaining);

    char seg_time[128], seg_count[128];
    to_segment_digits(seg_time, sizeof(seg_time), time_str);
    to_segment_digits(seg_count, sizeof(seg_count), count_str);

    set_cursor(c, x, y - 1); set_color_fg(c, 255, 100, 100); send_client(c, seg_count);
    set_cursor(c, x + bw - 8, y - 1); set_color_fg(c, 100, 255, 100); send_client(c, seg_time);
    
    if (c->screen == SCREEN_LOGIN_USER) {
        set_color_fg(c, 0, 255, 255); c->angle += 0.2f; draw_ascii_text(c, (w - 42) / 2, y - 8, TXT_WELCOME_ART, c->angle);
        set_color_bg_hex(c, "202050"); set_color_fg(c, 200, 200, 255); draw_box(c, x, y, bw, bh, TXT_BOX_LOGIN);
        set_cursor(c, x + 2, y + 2); send_client(c, TXT_PROMPT_USER); set_color_fg(c, 255, 255, 255); send_client(c, c->input_buffer);
    } else {
        set_color_bg_hex(c, "202050"); set_color_fg(c, 200, 200, 255); 
        draw_box(c, x, y, bw, bh, c->screen == SCREEN_LOGIN_CODE ? TXT_BOX_VERIFY : (c->is_new_user ? TXT_BOX_REGISTER : TXT_BOX_LOGIN));
        set_cursor(c, x + 2, y + 2); 
        if (c->screen == SCREEN_LOGIN_CODE) send_client(c, TXT_PROMPT_CODE);
        else send_client(c, c->is_new_user ? TXT_PROMPT_SET_PASS : TXT_PROMPT_PASS);
        set_color_fg(c, 255, 255, 255); 
        if (c->screen == SCREEN_LOGIN_CODE) send_client(c, c->input_buffer);
        else for(int i=0; i<c->input_len; i++) send_client(c, "*");
    }
}

void handle_input_login(Client *c, KeyCode key, char ch) {
    if (c->screen == SCREEN_LOGIN_USER) {
        if (key == KEY_ENTER && c->input_len > 0) {
            snprintf(c->username, sizeof(c->username), "%.*s", (int)sizeof(c->username) - 1, c->input_buffer);
            
            if (check_blacklist(c->username)) {
                //piper_speak(c, "Account permanently deleted.", 0);
                show_extro(c, "Your account has been permanently deleted.", "extro_deleted.txt");
                c->active = 0;
                return;
            }
            
            int uid = check_user_exists(c->username, c);
            if (uid != -1) {
                c->user_id = uid; c->is_new_user = 0;
                snprintf(c->password_buffer, sizeof(c->password_buffer), "%.*s", (int)sizeof(c->password_buffer) - 1, c->msg);
                if (!c->is_validated && !c->is_admin) {
                    c->screen = SCREEN_LOGIN_CODE;
                } else {
                    c->screen = SCREEN_LOGIN_PASS;
                }
                memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
            } else {
                // Check email format
                if (strchr(c->username, '@') && strchr(c->username, '.')) {
                    c->is_new_user = 1;
                    c->screen = SCREEN_LOGIN_PASS; memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
                } else {
                    piper_speak(c, "Invalid E-Mail format", 0);
                    memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
                }
            }
        } else if (key == KEY_BACKSPACE && c->input_len > 0) c->input_buffer[--c->input_len] = '\0';
        else if (key == KEY_CHAR && c->input_len < 32) { c->input_buffer[c->input_len++] = ch; c->input_buffer[c->input_len] = '\0'; }
    } else if (c->screen == SCREEN_LOGIN_CODE) {
        if (key == KEY_ENTER) {
            if (strcmp(c->input_buffer, c->verification_code) == 0) {
                db_verify_user(c);
                login_success(c);
            } else {
                piper_speak(c, "Wrong code", 0);
            }
            memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
        } else if (key == KEY_BACKSPACE && c->input_len > 0) c->input_buffer[--c->input_len] = '\0';
        else if (key == KEY_CHAR && c->input_len < 32) { c->input_buffer[c->input_len++] = ch; c->input_buffer[c->input_len] = '\0'; }
    } else {
        if (key == KEY_ENTER) {
            if (c->is_new_user) { 
                snprintf(c->verification_code, sizeof(c->verification_code), "%06d", rand() % 900000 + 100000);
                c->user_id = create_user(c, c->input_buffer);
                if (c->user_id <= 0) {
                    piper_speak(c, "Error creating user", 0);
                    c->screen = SCREEN_LOGIN_USER;
                    return;
                }
                char body[1024];
                snprintf(body, sizeof(body), "**********************************\n*    Welcome to Terrae Preta     *\n**********************************\n\nYour account has been created.\nPassword: %s\nVerification Code: %s\n\nYou have 1 minute of preview access.\nPlease login again to verify your email.", c->input_buffer, c->verification_code);
                send_email_async(c->username, "Welcome to Terrae Preta", body);
                c->is_validated = 0;
                login_success(c); 
            }
            else {
                if (strcmp(c->input_buffer, c->password_buffer) == 0) {
                    int double_login = 0;
                    pthread_mutex_lock(&clients_mutex);
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].active && clients[i].user_id == c->user_id && clients[i].fd != c->fd) { double_login = 1; break; }
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    if (double_login) { 
                        piper_speak(c, TTS_DENIED, 0); 
                        c->screen = SCREEN_LOGIN_USER; 
                        c->user_id = 0;
                        c->is_admin = 0;
                    }
                    else {
                        if (!c->is_admin && c->valid_until <= 0) {
                            piper_speak(c, "No credit left.", 0);
                            c->screen = SCREEN_LOGIN_USER; c->user_id = 0; c->is_admin = 0;
                        } else login_success(c);
                    }
                } else { 
                    piper_speak(c, TTS_DENIED, 0); 
                    c->screen = SCREEN_LOGIN_USER; 
                    c->user_id = 0;
                    c->is_admin = 0;
                }
            }
            memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
        } else if (key == KEY_BACKSPACE && c->input_len > 0) c->input_buffer[--c->input_len] = '\0';
        else if (key == KEY_CHAR && c->input_len < 32) { c->input_buffer[c->input_len++] = ch; c->input_buffer[c->input_len] = '\0'; }
    }
}