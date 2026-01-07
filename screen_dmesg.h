#ifndef SCREEN_DMESG_H
#define SCREEN_DMESG_H
#include "common.h"
void render_dmesg(Client *c);
void handle_input_dmesg(Client *c, KeyCode key, char ch);
#endif