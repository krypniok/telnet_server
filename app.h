#ifndef APP_H
#define APP_H

#include "common.h"

void piper_speak(Client *c, const char *text, float speed);
void oabeep_play(Client *c, const char *tokens);
void handle_client_logic(Client *c, KeyCode key, char ch);
void render_client(Client *c);
void show_extro(Client *c, const char *tts_msg, const char *txt_file);
void show_extro_fail(Client *c);
void add_to_blacklist(const char *email);
int check_blacklist(const char *email);

#endif