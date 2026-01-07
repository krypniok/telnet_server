#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include "screen_admin.h"
#include "gui.h"
#include "db.h"
#include "app.h"

void render_admin(Client *c) {
    set_color_bg_hex(c, "101020");
    clear_screen(c);
    
    int w = (c->width > 0) ? c->width : 80;
    int h = (c->height > 0) ? c->height : 24;
    
    // Layout
    int list_w = 25;
    int form_x = list_w + 2;
    int form_w = w - list_w - 3;
    
    // Draw Search
    draw_box(c, 1, 2, list_w, 3, TXT_ADMIN_SEARCH);
    set_cursor(c, 2, 3);
    if (c->admin_focus == 2) {
        set_color_fg(c, 255, 255, 0);
        send_client(c, TXT_PROMPT_CHAT);
        send_client(c, c->input_buffer);
        struct timeval tv; gettimeofday(&tv, NULL);
        if ((tv.tv_sec * 2 + tv.tv_usec / 500000) % 2 == 0) send_client(c, "█");
    } else {
        set_color_fg(c, 100, 100, 100);
        send_client(c, TXT_ADMIN_LOAD_HINT);
    }
    
    // Draw List
    draw_box(c, 1, 5, list_w, h - 6, TXT_ADMIN_USERS);
    for (int i = 0; i < c->admin_user_count; i++) {
        if (5 + 1 + i >= h - 2) break; // Clip
        set_cursor(c, 3, 6 + i);
        
        if (c->admin_focus == 0 && c->admin_list_idx == i) {
            set_color_bg_hex(c, "303050");
            set_color_fg(c, 255, 255, 0);
        } else {
            set_color_bg_hex(c, "101020");
            set_color_fg(c, 200, 200, 200);
        }
        
        char buf[32];
        snprintf(buf, sizeof(buf), "%-20.20s", c->admin_user_list[i]);
        send_client(c, buf);
    }
    
    // Draw Form
    draw_box(c, form_x, 2, form_w, h - 3, TXT_ADMIN_PROFILE);
    
    const char *labels[] = {LBL_EMAIL, LBL_FIRSTNAME, LBL_LASTNAME, LBL_STREET, LBL_ZIP, LBL_CITY, LBL_COUNTRY, LBL_PHONE, LBL_BIRTHDATE, LBL_POINTS, LBL_VALID_UNTIL, LBL_VALIDATED, LBL_CODE};
    char points_buf[32];
    snprintf(points_buf, sizeof(points_buf), "%d", c->continuum_points);
    char valid_buf[32];
    snprintf(valid_buf, sizeof(valid_buf), "%lld", c->profile.valid_until);
    char validated_buf[32];
    snprintf(validated_buf, sizeof(validated_buf), "%d", c->profile.is_validated);
    char *values[] = {c->profile.email, c->profile.firstname, c->profile.lastname, c->profile.street, c->profile.zip, c->profile.city, c->profile.country, c->profile.phone, c->profile.birthdate, points_buf, valid_buf, validated_buf, c->profile.verification_code};
    int num_fields = 13;

    if (c->admin_search_user[0] != '\0') {
        set_cursor(c, form_x + 2, 3);
        set_color_fg(c, 0, 255, 255);
        char title[80];
        snprintf(title, sizeof(title), TXT_ADMIN_USER_FMT, c->admin_search_user);
        send_client(c, title);
        
        for (int i = 0; i < num_fields; i++) {
            int y = 5 + i;
            set_cursor(c, form_x + 2, y);
            
            if (c->admin_focus == 1 && c->admin_field_idx == i) {
                set_color_fg(c, 255, 255, 0);
                send_client(c, TXT_PROMPT_CHAT);
            } else {
                set_color_fg(c, 150, 150, 150);
                send_client(c, "  ");
            }
            
            char line[128];
            snprintf(line, sizeof(line), "%-10s: %s", labels[i], values[i]);
            send_client(c, line);
            
            // Editing overlay
            if (c->admin_focus == 1 && c->admin_mode == 1 && c->admin_field_idx == i) {
                set_cursor(c, form_x + 16, y);
                set_color_bg_hex(c, "505000");
                set_color_fg(c, 255, 255, 255);
                send_client(c, c->input_buffer);
                
                // Blinking cursor
                struct timeval tv;
                gettimeofday(&tv, NULL);
                if ((tv.tv_sec * 2 + tv.tv_usec / 500000) % 2 == 0) {
                    send_client(c, "█");
                } else {
                    send_client(c, "_");
                }
                set_color_bg_hex(c, "101020");
            }
        }
        
        // Save Button
        set_cursor(c, form_x + 2, 5 + num_fields + 1);
        if (c->admin_focus == 1 && c->admin_field_idx == num_fields) set_color_fg(c, 0, 255, 0); 
        else set_color_fg(c, 255, 255, 255);
        send_client(c, TXT_BTN_SAVE);

        // Delete Button
        set_cursor(c, form_x + 20, 5 + num_fields + 1);
        if (c->admin_focus == 1 && c->admin_field_idx == num_fields + 1) set_color_fg(c, 255, 0, 0); 
        else set_color_fg(c, 150, 50, 50);
        send_client(c, TXT_BTN_DELETE);
        
    } else {
        set_cursor(c, form_x + 2, 4);
        set_color_fg(c, 150, 150, 150);
        send_client(c, TXT_ADMIN_SELECT_HINT);
    }
}

void handle_input_admin(Client *c, KeyCode key, char ch) {
    if (key == KEY_TAB) {
        c->admin_focus = (c->admin_focus + 1) % 3;
        c->admin_mode = 0; // Stop editing when switching focus
        memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
        return;
    }

    if (c->admin_focus == 0) { // List Focus
        if (key == KEY_UP) {
            if (c->admin_list_idx > 0) c->admin_list_idx--;
        } else if (key == KEY_DOWN) {
            if (c->admin_list_idx < c->admin_user_count - 1) c->admin_list_idx++;
        } else if (key == KEY_ENTER) {
            if (c->admin_user_count > 0) {
                snprintf(c->admin_search_user, sizeof(c->admin_search_user), "%s", c->admin_user_list[c->admin_list_idx]);
                db_load_user_profile(c->admin_search_user, c);
                c->admin_focus = 1; // Switch to form
                c->admin_field_idx = 0;
            }
        } else if (key == KEY_DELETE) {
            if (c->admin_user_count > 0) {
                char user_to_delete[64];
                snprintf(user_to_delete, sizeof(user_to_delete), "%s", c->admin_user_list[c->admin_list_idx]);
                if (strcmp(user_to_delete, c->username) == 0) {
                    piper_speak(c, TTS_NO_SELF_DEL, 0);
                    return;
                }
                if (db_delete_user(user_to_delete)) {
                    add_to_blacklist(user_to_delete);
                    piper_speak(c, TTS_DELETED, 0);
                    db_get_user_list(c);
                    if (c->admin_list_idx >= c->admin_user_count && c->admin_list_idx > 0) c->admin_list_idx--;
                    if (strcmp(c->admin_search_user, user_to_delete) == 0) c->admin_search_user[0] = '\0';
                }
            }
        }
    } else if (c->admin_focus == 2) { // Search Focus
        if (key == KEY_ENTER && c->input_len > 0) {
            snprintf(c->admin_search_user, sizeof(c->admin_search_user), "%.63s", c->input_buffer);
            if (db_load_user_profile(c->admin_search_user, c)) {
                c->admin_focus = 1; // Switch to form
                c->admin_field_idx = 0;
            } else {
                piper_speak(c, TTS_NOT_FOUND, 0);
            }
            memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
        } else if (key == KEY_BACKSPACE && c->input_len > 0) c->input_buffer[--c->input_len] = '\0';
        else if (key == KEY_CHAR && c->input_len < 63) { c->input_buffer[c->input_len++] = ch; c->input_buffer[c->input_len] = '\0'; }
    } else { // Form Focus
        int num_fields = 13;
        if (c->admin_mode == 0) { // Navigation
            int max_idx = num_fields + 2; // +1 Save, +1 Delete
            if (key == KEY_UP) c->admin_field_idx = (c->admin_field_idx - 1 + max_idx) % max_idx;
            else if (key == KEY_DOWN) c->admin_field_idx = (c->admin_field_idx + 1) % max_idx;
            else if (key == KEY_ENTER) {
                if (c->admin_field_idx == num_fields) { // Save
                    if (db_save_user_profile(c->admin_search_user, c)) {
                        piper_speak(c, TTS_SAVED, 0);
                        
                        // Apply changes to active client and disconnect if invalidated
                        pthread_mutex_lock(&clients_mutex);
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (clients[i].active && strcmp(clients[i].username, c->admin_search_user) == 0) {
                                clients[i].is_validated = c->profile.is_validated;
                                clients[i].valid_until = c->profile.valid_until;
                                if (clients[i].is_validated == 0) clients[i].pending_disconnect = "Account invalidated by admin";
                                break;
                            }
                        }
                        pthread_mutex_unlock(&clients_mutex);

                        // Update search user in case email changed
                        snprintf(c->admin_search_user, sizeof(c->admin_search_user), "%s", c->profile.email);
                        db_get_user_list(c); // Refresh list
                    } else {
                        piper_speak(c, TTS_SAVE_ERR, 0);
                    }
                } else if (c->admin_field_idx == num_fields + 1) { // Delete
                    if (strcmp(c->admin_search_user, c->username) == 0) {
                        piper_speak(c, TTS_NO_SELF_DEL, 0);
                        return;
                    }
                    if (db_delete_user(c->admin_search_user)) {
                        add_to_blacklist(c->admin_search_user);
                        piper_speak(c, TTS_DELETED, 0);
                        
                        // Instant disconnect deleted user
                        pthread_mutex_lock(&clients_mutex);
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if (clients[i].active && strcmp(clients[i].username, c->admin_search_user) == 0) {
                                clients[i].pending_disconnect = "Account deleted by admin";
                                clients[i].pending_disconnect_file = "extro_deleted.txt";
                                break;
                            }
                        }
                        pthread_mutex_unlock(&clients_mutex);

                        c->admin_search_user[0] = '\0';
                        c->admin_focus = 0;
                        db_get_user_list(c);
                    }
                } else { // Edit
                    c->admin_mode = 1;
                    memset(c->input_buffer, 0, sizeof(c->input_buffer)); c->input_len = 0;
                    char points_buf[32]; snprintf(points_buf, sizeof(points_buf), "%d", c->continuum_points);
                    char valid_buf[32]; snprintf(valid_buf, sizeof(valid_buf), "%lld", c->profile.valid_until);
                    char validated_buf[32]; snprintf(validated_buf, sizeof(validated_buf), "%d", c->profile.is_validated);
                    char *values[] = {c->profile.email, c->profile.firstname, c->profile.lastname, c->profile.street, c->profile.zip, c->profile.city, c->profile.country, c->profile.phone, c->profile.birthdate, points_buf, valid_buf, validated_buf, c->profile.verification_code};
                    snprintf(c->input_buffer, sizeof(c->input_buffer), "%s", values[c->admin_field_idx]);
                    c->input_len = strlen(c->input_buffer);
                }
            }
        } else { // Editing
            if (key == KEY_ENTER) {
                char *values[] = {c->profile.email, c->profile.firstname, c->profile.lastname, c->profile.street, c->profile.zip, c->profile.city, c->profile.country, c->profile.phone, c->profile.birthdate};
                if (c->admin_field_idx < 9) snprintf(values[c->admin_field_idx], 64, "%.63s", c->input_buffer);
                else if (c->admin_field_idx == 9) c->continuum_points = atoi(c->input_buffer);
                else if (c->admin_field_idx == 10) c->profile.valid_until = atoll(c->input_buffer);
                else if (c->admin_field_idx == 11) c->profile.is_validated = atoi(c->input_buffer);
                else if (c->admin_field_idx == 12) snprintf(c->profile.verification_code, sizeof(c->profile.verification_code), "%.15s", c->input_buffer);
                c->admin_mode = 0;
            } else if (key == KEY_ESCAPE) {
                c->admin_mode = 0;
            } else if (key == KEY_BACKSPACE && c->input_len > 0) c->input_buffer[--c->input_len] = '\0';
            else if (key == KEY_CHAR && c->input_len < 63) { c->input_buffer[c->input_len++] = ch; c->input_buffer[c->input_len] = '\0'; }
        }
    }
}