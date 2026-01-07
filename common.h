#ifndef COMMON_H
#define COMMON_H

#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <netinet/in.h>
#include "translation_de.h"

#define PORT 12345
#define MAX_CLIENTS 1000
#define BUFFER_SIZE 1024

// ANSI Colors
#define ANSI_RESET "\033[0m"
#define ANSI_CLS "\033[2J\033[H"
#define ANSI_HIDE_CURSOR "\033[?25l"
#define ANSI_SHOW_CURSOR "\033[?25h"

typedef enum {
    KEY_NONE, KEY_ENTER, KEY_BACKSPACE, KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_ESCAPE, KEY_CHAR,
    KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
    KEY_TAB, KEY_INSERT, KEY_DELETE, KEY_HOME, KEY_END, KEY_PGUP, KEY_PGDOWN
} KeyCode;

typedef enum {
    SCREEN_LOGIN_USER = 0,
    SCREEN_LOGIN_PASS = 1,
    SCREEN_LOGIN_CODE = 2,
    SCREEN_INTRO = 3,
    SCREEN_CARD = 4,
    SCREEN_BOX = 5,
    SCREEN_CUBE = 6,
    SCREEN_KEY_DEBUG = 7,
    SCREEN_DMESG = 8,
    SCREEN_CHAT = 9,
    SCREEN_POKER = 10,
    SCREEN_ADMIN = 11
} ScreenState;

typedef struct {
    int fd;
    int active;
    time_t last_active;
    ScreenState screen;
    
    // User Profile Data
    struct {
        char email[64];
        char firstname[64];
        char lastname[64];
        char street[64];
        char zip[16];
        char city[64];
        char country[64];
        char phone[32];
        char birthdate[16];
        long long valid_until;
        int is_validated;
        char verification_code[16];
    } profile;
    int continuum_points;

    int user_id;
    char username[64];
    char input_buffer[256];
    int input_len;
    char msg[256];
    char password_buffer[64];
    int is_new_user;
    int is_admin;
    int is_validated;
    char verification_code[16];
    long long valid_until;
    time_t login_time;
    long upload;
    long download;
    float angle;
    float zoom;
    int xcmd_status; // 0=pending, 1=active, 2=rejected
    time_t connect_time;
    int greeting_played;
    int cube_render_mode; // 0 = Wireframe, 1 = Solid
    
    // Rendering
    int needs_redraw;
    struct timeval last_render;
    const char *pending_disconnect;
    const char *pending_disconnect_file;
    
    // Telnet State
    int telnet_state;
    int telnet_cmd;
    unsigned char sub_buffer[10];
    int sub_len;
    int width;
    int height;
    
    // GUI State
    int list_selected;

    // Debug State
    char debug_last_key[64];
    char debug_utf8_buf[8];
    int debug_utf8_len;

    // Output Buffer
    char out_buf[65536];
    int out_len;
    
    // Admin Screen State
    int admin_mode; // 0=Search, 1=Form, 2=EditingField
    char admin_search_user[64];
    int admin_field_idx;
    int admin_focus; // 0=List, 1=Form
    char admin_user_list[50][64]; // Cache for user list
    int admin_user_count;
    int admin_list_idx;
} Client;

extern Client clients[MAX_CLIENTS];
extern pthread_mutex_t clients_mutex;

#define MAX_CHAT_HISTORY 50
typedef struct {
    char username[32];
    char message[200];
    char timestamp[10];
} ChatMessage;

extern ChatMessage chat_history[MAX_CHAT_HISTORY];
extern int chat_head;
extern pthread_mutex_t chat_mutex;

extern int CONF_TIMEOUT_LOGIN;
extern int CONF_TIMEOUT_LOGGED_IN;
extern int CONF_TICK_RATE_US;
extern int CONF_FPS_LOGIN;
extern int CONF_FPS_CUBE;
extern int CONF_FPS_INTRO;
extern float CONF_TTS_GLOBAL_SPEED;
extern int CONF_FPS_DMESG;
extern int CONF_FPS_CHAT;
extern int CONF_CREDIT_NEW_USER;
extern int CONF_CREDIT_VERIFIED;
extern int CONF_BAIL_OUT_IF_NO_XCMD;

#endif