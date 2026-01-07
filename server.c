#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <time.h>
#include <sqlite3.h>
#include <ctype.h>
#include <math.h>
#include <sys/time.h>
#include <stddef.h>
#include <pthread.h>
#include "common.h"
#include "gui.h"
#include "db.h"
#include "app.h"

// --- Configuration & Timings ---
int CONF_TIMEOUT_LOGIN = 15;
int CONF_TIMEOUT_LOGGED_IN = 300; // 5 minutes
int CONF_TICK_RATE_US = 20000; // 20ms base tick
int CONF_FPS_LOGIN = 10;
int CONF_FPS_CUBE = 20;
int CONF_FPS_INTRO = 1;
int CONF_FPS_DMESG = 1;
int CONF_FPS_CHAT = 1;
float CONF_TTS_GLOBAL_SPEED = 3.0;
int CONF_CREDIT_NEW_USER = 60;
int CONF_CREDIT_VERIFIED = 3600;
int CONF_BAIL_OUT_IF_NO_XCMD = 0;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

ChatMessage chat_history[MAX_CHAT_HISTORY];
int chat_head = 0;
pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_telnet(Client *c, unsigned char byte) {
    // Simple state machine for IAC DO/WILL/SB
    switch (c->telnet_state) {
        case 0:
            if (byte == 255) c->telnet_state = 1;
            break;
        case 1: // IAC received
            if (byte >= 251 && byte <= 254) {
                c->telnet_state = 2; // WILL/WONT/DO/DONT
                c->telnet_cmd = byte; // Store command
            } else if (byte == 250) { // SB
                c->sub_len = 0;
                c->telnet_state = 3;
            } else {
                c->telnet_state = 0;
            }
            break;
        case 2: // Option code
            if (byte == 31) { // NAWS
                unsigned char resp[] = {255, 253, 31}; // IAC DO NAWS
                if (send(c->fd, resp, 3, 0) > 0) c->upload += 3;
                // Note: Direct send bypasses buffer, but for small IAC it's usually fine or we should flush before.
            }
            else if (byte == 200) { // XCMD (200)
                if (c->telnet_cmd == 253) { // DO
                    c->xcmd_status = 1;
                    printf(LOG_XCMD_ENABLED, c->fd);
                    if (!c->greeting_played) {
                        piper_speak(c, TTS_IDENTIFY, 0);
                        c->greeting_played = 1;
                    }
                } else {
                    c->xcmd_status = 2; // DONT or WONT
                }
            }
            c->telnet_state = 0;
            break;
        case 3: // SB content
            if (byte == 255) c->telnet_state = 4;
            else if (c->sub_len < 10) c->sub_buffer[c->sub_len++] = byte;
            break;
        case 4: // IAC inside SB
            if (byte == 240) { // SE
                if (c->sub_len >= 4 && c->sub_buffer[0] == 31) {
                    c->width = (c->sub_buffer[1] << 8) | c->sub_buffer[2];
                    c->height = (c->sub_buffer[3] << 8) | c->sub_buffer[4];
                    printf(LOG_RESIZED, c->fd, c->width, c->height);
                    c->needs_redraw = 1;
                }
                c->telnet_state = 0;
            } else {
                c->telnet_state = 3; // Was not SE, back to content
            }
            break;
    }
}

void process_buffer(Client *c, unsigned char *buf, int len) {
    for (int i = 0; i < len; i++) {
        unsigned char b = buf[i];
        
        // Telnet Negotiation
        if (c->telnet_state > 0 || b == 255) {
            handle_telnet(c, b);
            continue;
        }

        // ANSI / Key Parsing
        KeyCode key = KEY_CHAR;
        char ch = b;

        if (b == 0x1B) { // ESC
            int handled = 0;
            if (i + 1 < len) {
                if (buf[i+1] == '[') {
                    if (i + 2 < len) {
                        switch(buf[i+2]) {
                            case 'A': key = KEY_UP; i+=2; handled=1; break;
                            case 'B': key = KEY_DOWN; i+=2; handled=1; break;
                            case 'C': key = KEY_RIGHT; i+=2; handled=1; break;
                            case 'D': key = KEY_LEFT; i+=2; handled=1; break;
                            case 'H': key = KEY_HOME; i+=2; handled=1; break;
                            case 'F': key = KEY_END; i+=2; handled=1; break;
                            case '1': // F1-F8 or Home [1~
                                if (i+3 < len && buf[i+3] == '~') {
                                    key = KEY_HOME; i+=3; handled=1;
                                } else if (i+4 < len && buf[i+4] == '~') {
                                    switch(buf[i+3]) {
                                        case '1': key = KEY_F1; break; // [11~
                                        case '2': key = KEY_F2; break; // [12~
                                        case '3': key = KEY_F3; break; // [13~
                                        case '4': key = KEY_F4; break; // [14~
                                        case '5': key = KEY_F5; break; // [15~
                                        case '7': key = KEY_F6; break; // [17~
                                        case '8': key = KEY_F7; break; // [18~
                                        case '9': key = KEY_F8; break; // [19~
                                    }
                                    i+=4; handled=1;
                                }
                                break;
                            case '2': // F9-F12 or Insert [2~
                                if (i+3 < len && buf[i+3] == '~') {
                                    key = KEY_INSERT; i+=3; handled=1;
                                } else if (i+4 < len && buf[i+4] == '~') {
                                    switch(buf[i+3]) {
                                        case '0': key = KEY_F9; break;  // [20~
                                        case '1': key = KEY_F10; break; // [21~
                                        case '3': key = KEY_F11; break; // [23~
                                        case '4': key = KEY_F12; break; // [24~
                                    }
                                    i+=4; handled=1;
                                }
                                break;
                            case '3': // Delete [3~
                                if (i+3 < len && buf[i+3] == '~') {
                                    key = KEY_DELETE; i+=3; handled=1;
                                }
                                break;
                            case '4': // End [4~
                                if (i+3 < len && buf[i+3] == '~') {
                                    key = KEY_END; i+=3; handled=1;
                                }
                                break;
                            case '5': // PgUp [5~
                                if (i+3 < len && buf[i+3] == '~') {
                                    key = KEY_PGUP; i+=3; handled=1;
                                }
                                break;
                            case '6': // PgDn [6~
                                if (i+3 < len && buf[i+3] == '~') {
                                    key = KEY_PGDOWN; i+=3; handled=1;
                                }
                                break;
                        }
                    }
                } else if (buf[i+1] == 'O') { // VT100 F1-F4
                    if (i + 2 < len) {
                        switch(buf[i+2]) {
                            case 'P': key = KEY_F1; i+=2; handled=1; break;
                            case 'Q': key = KEY_F2; i+=2; handled=1; break;
                            case 'R': key = KEY_F3; i+=2; handled=1; break;
                            case 'S': key = KEY_F4; i+=2; handled=1; break;
                        }
                    }
                }
            }
            
            if (!handled) {
                key = KEY_ESCAPE;
            }
            ch = 0;
        } else if (b == 0x0D || b == 0x0A) {
            key = KEY_ENTER;
            ch = 0;
        } else if (b == 0x7F || b == 0x08) {
            key = KEY_BACKSPACE;
            ch = 0;
        } else if (b == 0x09) {
            key = KEY_TAB;
            ch = 0;
        } else if (b == 0) {
            key = KEY_NONE; // Ignore NUL (often sent after CR in Telnet)
        } else if (b < 32) {
            // Allow control codes (CTRL+A etc) to pass as KEY_CHAR for debug
            key = KEY_CHAR;
            ch = b;
        }

        if (key != KEY_NONE) {
            handle_client_logic(c, key, ch);
        }
    }
}

void set_raw_mode(Client *c) {
    // IAC WILL SUPPRESS-GO-AHEAD, IAC WILL ECHO, IAC DO NAWS
    unsigned char cmd[] = {
        255, 251, 3, 
        255, 251, 1, 
        255, 253, 1,
        255, 253, 31,
        255, 251, 200 // IAC WILL 200 (XCMD)
    };
    if (send(c->fd, cmd, sizeof(cmd), 0) > 0) c->upload += sizeof(cmd);
}

void *client_thread(void *arg) {
    Client *c = (Client *)arg;
    
    // Init sequence
    set_raw_mode(c);
    
    c->last_active = time(NULL);
    c->connect_time = time(NULL);
    c->xcmd_status = 0; // Pending

    // Negotiation Wait Loop (max 3 seconds)
    while (c->active && c->xcmd_status == 0) {
        if (time(NULL) - c->connect_time > 3) {
            c->xcmd_status = 2; // Timeout
            break;
        }
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(c->fd, &readfds);
        struct timeval tv = {0, 10000}; // 10ms
        
        if (select(c->fd + 1, &readfds, NULL, NULL, &tv) > 0) {
            unsigned char buffer[BUFFER_SIZE];
            int valread = read(c->fd, buffer, BUFFER_SIZE);
            if (valread <= 0) { c->active = 0; break; }
            process_buffer(c, buffer, valread);
        }
    }

    if (c->active) {
        if (c->xcmd_status != 1 && CONF_BAIL_OUT_IF_NO_XCMD) {
            show_extro_fail(c);
            c->active = 0;
        } else {
            // Success!
            send_client(c, ANSI_HIDE_CURSOR);
            clear_screen(c);
            
    		gettimeofday(&c->last_render, NULL);
            while (c->active) {
                // 0. Check Pending Disconnect (from Admin)
                if (c->pending_disconnect) {
                    const char *file = c->pending_disconnect_file ? c->pending_disconnect_file : "extro_kicked.txt";
                    show_extro(c, c->pending_disconnect, file);
                    c->active = 0;
                    break;
                }

                // 1. Check Timeout
                time_t now_sec = time(NULL);
                int limit = (c->screen == SCREEN_LOGIN_USER || c->screen == SCREEN_LOGIN_PASS || c->screen == SCREEN_LOGIN_CODE) 
                            ? CONF_TIMEOUT_LOGIN : CONF_TIMEOUT_LOGGED_IN;
                
                if (!c->is_admin && now_sec - c->last_active > limit) {
                    printf(LOG_TIMEOUT, c->fd);
                    if (c->screen == SCREEN_LOGIN_USER || c->screen == SCREEN_LOGIN_PASS) {
                        show_extro(c, TTS_TIMEOUT, "extro_timeout.txt");
                    } else {
                        show_extro(c, TTS_BYE, "extro_timeout.txt");
                    }
                    c->active = 0;
                    break;
                }

                // Credit Check (Kick if time runs out)
                if (c->screen != SCREEN_LOGIN_USER && c->screen != SCREEN_LOGIN_PASS && c->screen != SCREEN_LOGIN_CODE &&
                    c->user_id > 0 && !c->is_admin) {
                    long long used_time = (long long)(now_sec - c->login_time);
                    if (c->valid_until - used_time <= 0) {
                        show_extro(c, "Dein Abo ist abgelaufen ! Auf Wiedersehen.", "extro_expired.txt");
                        c->active = 0;
                        break;
                    }
                }

                // 2. Select for Input (Thread-local)
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(c->fd, &readfds);
                
                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = CONF_TICK_RATE_US;

                int activity = select(c->fd + 1, &readfds, NULL, NULL, &tv);

                if (activity > 0 && FD_ISSET(c->fd, &readfds)) {
                    unsigned char buffer[BUFFER_SIZE];
                    int valread = read(c->fd, buffer, BUFFER_SIZE);
                    
                    if (valread <= 0) {
                        c->active = 0; // Disconnect
                        break;
                    }
                    
                    c->last_active = time(NULL);
                    c->download += valread;
                    process_buffer(c, buffer, valread);
                }

                // 3. Render Logic
                if (c->active) {
                    int target_fps = 0;
                    if (c->screen == SCREEN_CUBE) target_fps = CONF_FPS_CUBE;
                    else if (c->screen == SCREEN_LOGIN_USER || c->screen == SCREEN_LOGIN_PASS || c->screen == SCREEN_LOGIN_CODE) target_fps = CONF_FPS_LOGIN;
                    else if (c->screen == SCREEN_INTRO) target_fps = CONF_FPS_INTRO;
                    else if (c->screen == SCREEN_DMESG) target_fps = CONF_FPS_DMESG;
                    else if (c->screen == SCREEN_CHAT) target_fps = CONF_FPS_CHAT;

                    struct timeval now;
                    gettimeofday(&now, NULL);
                    long long diff = (now.tv_sec - c->last_render.tv_sec) * 1000LL + 
                                     (now.tv_usec - c->last_render.tv_usec) / 1000LL;

                    if ((target_fps > 0 && diff >= 1000 / target_fps) || c->needs_redraw) {
                        render_client(c);
                        flush_client(c);
                        c->last_render = now;
                        c->needs_redraw = 0;
                    }
                }
            }
        }
    }

    // Cleanup
    if (c->user_id > 0 && !c->is_admin && c->screen != SCREEN_LOGIN_USER && c->screen != SCREEN_LOGIN_PASS && c->screen != SCREEN_LOGIN_CODE) {
        // Deduct used time from credit
        time_t now = time(NULL);
        long long used = (long long)(now - c->login_time);
        c->valid_until -= used;
        if (c->valid_until < 0) c->valid_until = 0;
    }
    save_user_stats(c);
    close(c->fd);
    printf(LOG_EXIT, c->fd);
    
    pthread_mutex_lock(&clients_mutex);
    c->active = 0; // Mark slot as free
    pthread_mutex_unlock(&clients_mutex);
    
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    srand(time(NULL));

    init_db();

    // Init clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = 0;
        clients[i].active = 0;
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 100) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf(LOG_LISTENING, PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &addrlen);
        
        if (new_socket >= 0) {
            printf(LOG_NEW_CONN, new_socket);
            
            pthread_mutex_lock(&clients_mutex);
            int found = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i].active) {
                    found = i;
                    break;
                }
            }
            
            if (found != -1) {
                Client *c = &clients[found];
                memset(c, 0, sizeof(Client));
                c->fd = new_socket;
                c->active = 1;
                c->screen = SCREEN_LOGIN_USER;
                c->needs_redraw = 1;
                
                pthread_t tid;
                if (pthread_create(&tid, NULL, client_thread, c) != 0) {
                    perror("pthread_create");
                    c->active = 0;
                    close(new_socket);
                } else {
                    pthread_detach(tid);
                }
            } else {
                printf(LOG_MAX_CLIENTS);
                close(new_socket);
            }
            pthread_mutex_unlock(&clients_mutex);
        }
    }

    return 0;
}