#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "screen_chat.h"
#include "gui.h"
#include "app.h"

void render_chat(Client *c) {
    int w = (c->width > 0) ? c->width : 80;
    int h = (c->height > 0) ? c->height : 24;
    
    set_color_bg(c, 10, 10, 10);
    clear_screen(c);
    
    int sidebar_w = 20;
    int chat_w = w - sidebar_w - 1;
    
    set_color_fg(c, 80, 80, 80);
    for (int y = 1; y < h; y++) {
        set_cursor(c, chat_w + 1, y);
        send_client(c, "│");
    }
    
    // Draw Credit Timer
    if (!c->is_admin) {
        time_t now = time(NULL);
        long long used = (long long)(now - c->login_time);
        long long remaining = c->valid_until - used;
        if (remaining < 0) remaining = 0;

        int hours = remaining / 3600;
        int mins = (remaining % 3600) / 60;
        int secs = remaining % 60;
        char time_str[32];
        if (hours > 0) snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, mins, secs);
        else snprintf(time_str, sizeof(time_str), "%02d:%02d", mins, secs);
        
        char seg_time[128];
        to_segment_digits(seg_time, sizeof(seg_time), time_str);
        
        set_cursor(c, chat_w + 3, 1);
        set_color_fg(c, 255, 200, 0);
        send_client(c, seg_time);
    }

    set_cursor(c, chat_w + 3, 2);
    set_color_fg(c, 0, 255, 255);
    send_client(c, TXT_ONLINE_USERS);
    
    pthread_mutex_lock(&clients_mutex);
    int y = 4;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].user_id > 0 && clients[i].screen != SCREEN_LOGIN_USER && clients[i].screen != SCREEN_LOGIN_PASS) {
            set_cursor(c, chat_w + 3, y++);
            if (y >= h) break;
            if (clients[i].is_admin) set_color_fg(c, 255, 0, 0);
            else if (clients[i].user_id == c->user_id) set_color_fg(c, 0, 255, 0);
            else set_color_fg(c, 200, 200, 200);
            char name_buf[32];
            snprintf(name_buf, sizeof(name_buf), "%.16s", clients[i].username);
            send_client(c, name_buf);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    
    int top_y = 1;
    if (c->xcmd_status != 1) {
        FILE *fp = fopen("extro_fail.txt", "r");
        if (fp) {
            char line[256];
            set_color_fg(c, 255, 50, 50);
            while (fgets(line, sizeof(line), fp)) {
                if (top_y >= h - 2) break;
                line[strcspn(line, "\r\n")] = 0;
                if ((int)strlen(line) > chat_w) line[chat_w] = 0;
                set_cursor(c, 1, top_y++);
                send_client(c, line);
            }
            fclose(fp);
            top_y++;
        }
    }

    int msg_y = h - 2;
    pthread_mutex_lock(&chat_mutex);
    int idx = chat_head - 1;
    if (idx < 0) idx = MAX_CHAT_HISTORY - 1;
    int count = 0;
    while (count < MAX_CHAT_HISTORY && msg_y >= top_y) {
        if (chat_history[idx].timestamp[0] != '\0') {
            set_cursor(c, 1, msg_y);
            char line[512];
            snprintf(line, sizeof(line), "\033[38;2;100;100;100m[%s] \033[38;2;255;255;0m<%s>\033[0m %s", 
                chat_history[idx].timestamp, chat_history[idx].username, chat_history[idx].message);
            send_client(c, line);
            msg_y--;
        } else break;
        idx--;
        if (idx < 0) idx = MAX_CHAT_HISTORY - 1;
        count++;
    }
    pthread_mutex_unlock(&chat_mutex);
    
    set_cursor(c, 1, h);
    set_color_bg_hex(c, "222222");
    char spacer[256]; int spacer_len = (w > 255) ? 255 : w; memset(spacer, ' ', spacer_len); spacer[spacer_len] = 0;
    send_client(c, spacer);
    set_cursor(c, 1, h); set_color_fg(c, 255, 255, 255); send_client(c, TXT_PROMPT_CHAT); send_client(c, c->input_buffer);
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if ((tv.tv_sec * 2 + tv.tv_usec / 500000) % 2 == 0) {
        send_client(c, "█");
    }
}

void handle_input_chat(Client *c, KeyCode key, char ch) {
    if (key == KEY_ENTER) {
        if (c->input_len > 0) {
            pthread_mutex_lock(&chat_mutex);
            time_t now = time(NULL); struct tm *t = localtime(&now);
            snprintf(chat_history[chat_head].timestamp, 10, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
            snprintf(chat_history[chat_head].username, 32, "%.31s", c->username);
            snprintf(chat_history[chat_head].message, 200, "%.199s", c->input_buffer);
            chat_head = (chat_head + 1) % MAX_CHAT_HISTORY;
            pthread_mutex_unlock(&chat_mutex);
            
            pthread_mutex_lock(&clients_mutex);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active && clients[i].user_id > 0 && clients[i].fd != c->fd) {
                    if (strstr(c->input_buffer, clients[i].username)) {
                        char buf[128];
                        snprintf(buf, sizeof(buf), TTS_MSG_PREFIX, c->username);
                        piper_speak(&clients[i], buf, 0);
                    }
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            
            memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
        }
    } else if (key == KEY_BACKSPACE && c->input_len > 0) c->input_buffer[--c->input_len] = '\0';
    else if (key == KEY_CHAR && c->input_len < 200) { c->input_buffer[c->input_len++] = ch; c->input_buffer[c->input_len] = '\0'; }
}