#ifndef DB_H
#define DB_H

#include "common.h"

void init_db();
int check_user_exists(const char *username, Client *c);
int create_user(Client *c, const char *password);
void save_user_stats(Client *c);
int db_load_user_profile(const char *username, Client *c);
int db_save_user_profile(const char *username, Client *c);
void db_get_user_list(Client *c);
int db_delete_user(const char *username);
void db_verify_user(Client *c);

#endif