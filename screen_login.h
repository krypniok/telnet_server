#ifndef SCREEN_LOGIN_H
#define SCREEN_LOGIN_H
#include "common.h"
void render_login(Client *c);
void handle_input_login(Client *c, KeyCode key, char ch);
#endif