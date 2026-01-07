#ifndef SCREEN_CHAT_H
#define SCREEN_CHAT_H
#include "common.h"
void render_chat(Client *c);
void handle_input_chat(Client *c, KeyCode key, char ch);
#endif