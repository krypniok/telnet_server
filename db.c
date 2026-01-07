#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "db.h"

static sqlite3 *db;

void init_db() {
    int rc = sqlite3_open("db.sqlite", &db);
    if (rc) {
        fprintf(stderr, LOG_DB_FAIL, sqlite3_errmsg(db));
        exit(1);
    }
    
    char *sql = "CREATE TABLE IF NOT EXISTS users ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "email TEXT UNIQUE NOT NULL,"
                "password TEXT DEFAULT '',"
                "is_admin INTEGER DEFAULT 0,"
                "upload INTEGER DEFAULT 0,"
                "download INTEGER DEFAULT 0,"
                "online INTEGER DEFAULT 0,"
                "created_at TEXT DEFAULT CURRENT_TIMESTAMP);";
    
    char *err_msg = 0;
    rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, LOG_SQL_ERR, err_msg);
        sqlite3_free(err_msg);
    }
    
    // Try to add columns if they don't exist (ignoring errors if they do)
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN firstname TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN lastname TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN street TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN zip TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN city TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN country TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN phone TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN birthdate TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN continuum_points INTEGER DEFAULT 0;", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN valid_until INTEGER DEFAULT 0;", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN verification_code TEXT DEFAULT '';", 0, 0, 0);
    sqlite3_exec(db, "ALTER TABLE users ADD COLUMN is_validated INTEGER DEFAULT 0;", 0, 0, 0);
    sqlite3_exec(db, "UPDATE users SET is_validated = 1 WHERE id = 1;", 0, 0, 0);
}

int check_user_exists(const char *username, Client *c) {
    sqlite3_stmt *stmt;
    int user_id = -1;
    
    const char *sql_select = "SELECT id, password, upload, download, is_admin, valid_until, verification_code, is_validated FROM users WHERE LOWER(email) = LOWER(?)";
    sqlite3_prepare_v2(db, sql_select, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user_id = sqlite3_column_int(stmt, 0);
        const char *stored_pass = (const char*)sqlite3_column_text(stmt, 1);
        c->upload = sqlite3_column_int64(stmt, 2);
        c->download = sqlite3_column_int64(stmt, 3);
        c->is_admin = sqlite3_column_int(stmt, 4);
        c->valid_until = sqlite3_column_int64(stmt, 5);
        snprintf(c->verification_code, sizeof(c->verification_code), "%s", (const char*)sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "");
        c->is_validated = sqlite3_column_int(stmt, 7);
        
        if (stored_pass) strncpy(c->msg, stored_pass, sizeof(c->msg)-1);
        else c->msg[0] = '\0';
    }
    sqlite3_finalize(stmt);
    return user_id;
}

int create_user(Client *c, const char *password) {
    sqlite3_stmt *stmt;
    int user_id = -1;
    int is_admin = 0;

    // Check if table is empty (first user becomes admin)
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users", -1, &stmt, 0);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_int(stmt, 0) == 0) {
            is_admin = 1;
        }
    }
    sqlite3_finalize(stmt);

    long long valid_until = CONF_CREDIT_NEW_USER; // Default credit for new users
    const char *sql_insert = "INSERT INTO users (email, password, is_admin, valid_until, verification_code, is_validated) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_prepare_v2(db, sql_insert, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, c->username, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, is_admin);
    sqlite3_bind_int64(stmt, 4, valid_until);
    sqlite3_bind_text(stmt, 5, c->verification_code, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 6, 0); // is_validated = 0
    
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        user_id = (int)sqlite3_last_insert_rowid(db);
    }
    sqlite3_finalize(stmt);
    
    printf(LOG_USER_CREATED, c->username, user_id, is_admin);

    // Fallback: Wenn dies der erste User ist (ID 1), erzwinge Admin-Rechte
    if (user_id == 1 && is_admin == 0) {
        is_admin = 1;
        sqlite3_exec(db, "UPDATE users SET is_admin = 1, is_validated = 1 WHERE id = 1", 0, 0, 0);
        printf(LOG_ADMIN_PROMO);
        c->is_validated = 1;
    }
    c->is_admin = is_admin;
    c->valid_until = valid_until;
    
    return user_id;
}

void db_verify_user(Client *c) {
    if (c->user_id <= 0) return;
    c->is_validated = 1;
    c->valid_until += CONF_CREDIT_VERIFIED;
    
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE users SET is_validated = 1, valid_until = ? WHERE id = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int64(stmt, 1, c->valid_until);
    sqlite3_bind_int(stmt, 2, c->user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void save_user_stats(Client *c) {
    if (c->user_id <= 0) return;
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE users SET upload = ?, download = ?, valid_until = ?, online = 0 WHERE id = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_int64(stmt, 1, c->upload);
    sqlite3_bind_int64(stmt, 2, c->download);
    sqlite3_bind_int64(stmt, 3, c->valid_until);
    sqlite3_bind_int(stmt, 4, c->user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    printf(LOG_STATS_SAVED, c->user_id, c->upload, c->download);
}

int db_load_user_profile(const char *username, Client *c) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT firstname, lastname, street, zip, city, country, phone, birthdate, valid_until, is_validated, verification_code FROM users WHERE email = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    int found = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = 1;
        snprintf(c->profile.email, sizeof(c->profile.email), "%s", username);
        snprintf(c->profile.firstname, sizeof(c->profile.firstname), "%s", (const char*)sqlite3_column_text(stmt, 0) ? (const char*)sqlite3_column_text(stmt, 0) : "");
        snprintf(c->profile.lastname, sizeof(c->profile.lastname), "%s", (const char*)sqlite3_column_text(stmt, 1) ? (const char*)sqlite3_column_text(stmt, 1) : "");
        snprintf(c->profile.street, sizeof(c->profile.street), "%s", (const char*)sqlite3_column_text(stmt, 2) ? (const char*)sqlite3_column_text(stmt, 2) : "");
        snprintf(c->profile.zip, sizeof(c->profile.zip), "%s", (const char*)sqlite3_column_text(stmt, 3) ? (const char*)sqlite3_column_text(stmt, 3) : "");
        snprintf(c->profile.city, sizeof(c->profile.city), "%s", (const char*)sqlite3_column_text(stmt, 4) ? (const char*)sqlite3_column_text(stmt, 4) : "");
        snprintf(c->profile.country, sizeof(c->profile.country), "%s", (const char*)sqlite3_column_text(stmt, 5) ? (const char*)sqlite3_column_text(stmt, 5) : "");
        snprintf(c->profile.phone, sizeof(c->profile.phone), "%s", (const char*)sqlite3_column_text(stmt, 6) ? (const char*)sqlite3_column_text(stmt, 6) : "");
        snprintf(c->profile.birthdate, sizeof(c->profile.birthdate), "%s", (const char*)sqlite3_column_text(stmt, 7) ? (const char*)sqlite3_column_text(stmt, 7) : "");
        c->profile.valid_until = sqlite3_column_int64(stmt, 8);
        c->profile.is_validated = sqlite3_column_int(stmt, 9);
        snprintf(c->profile.verification_code, sizeof(c->profile.verification_code), "%s", (const char*)sqlite3_column_text(stmt, 10) ? (const char*)sqlite3_column_text(stmt, 10) : "");
    }
    sqlite3_finalize(stmt);
    return found;
}

int db_save_user_profile(const char *username, Client *c) {
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE users SET firstname=?, lastname=?, street=?, zip=?, city=?, country=?, phone=?, birthdate=?, valid_until=?, is_validated=?, verification_code=? WHERE email=?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, c->profile.firstname, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, c->profile.lastname, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, c->profile.street, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, c->profile.zip, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, c->profile.city, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, c->profile.country, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, c->profile.phone, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, c->profile.birthdate, -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 9, c->profile.valid_until);
    sqlite3_bind_int(stmt, 10, c->profile.is_validated);
    sqlite3_bind_text(stmt, 11, c->profile.verification_code, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 12, username, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

void db_get_user_list(Client *c) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT email FROM users ORDER BY email ASC LIMIT 50";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    c->admin_user_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && c->admin_user_count < 50) {
        const char *email = (const char*)sqlite3_column_text(stmt, 0);
        if (email) {
            snprintf(c->admin_user_list[c->admin_user_count], sizeof(c->admin_user_list[0]), "%s", email);
            c->admin_user_count++;
        }
    }
    sqlite3_finalize(stmt);
}

int db_delete_user(const char *username) {
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM users WHERE email = ?";
    sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, username, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}