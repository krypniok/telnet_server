#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "screen_cube.h"
#include "gui.h"

typedef struct {
    int active;
    int r, g, b;
    float z;
} Pixel;

static void draw_line_buffer(Pixel *buffer, int w, int h, int x0, int y0, int x1, int y1, int r, int g, int b) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        if (x0 >= 0 && x0 < w && y0 >= 0 && y0 < h) {
            int idx = x0 * h + y0;
            buffer[idx].active = 1; buffer[idx].r = r; buffer[idx].g = g; buffer[idx].b = b;
        }
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err; if (e2 >= dy) { err += dy; x0 += sx; } if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void draw_triangle(Pixel *buffer, int w, int h, float *v0, float *v1, float *v2) {
    int minx = (int)floor(fmin(v0[0], fmin(v1[0], v2[0])));
    int maxx = (int)ceil(fmax(v0[0], fmax(v1[0], v2[0])));
    int miny = (int)floor(fmin(v0[1], fmin(v1[1], v2[1])));
    int maxy = (int)ceil(fmax(v0[1], fmax(v1[1], v2[1])));
    minx = (minx < 0) ? 0 : minx; miny = (miny < 0) ? 0 : miny;
    maxx = (maxx >= w) ? w - 1 : maxx; maxy = (maxy >= h) ? h - 1 : maxy;
    float area = (v1[1] - v2[1]) * (v0[0] - v2[0]) + (v2[0] - v1[0]) * (v0[1] - v2[1]);
    if (fabs(area) < 0.01f) return;
    for (int y = miny; y <= maxy; y++) {
        for (int x = minx; x <= maxx; x++) {
            float w0 = ((v1[1] - v2[1]) * (x - v2[0]) + (v2[0] - v1[0]) * (y - v2[1])) / area;
            float w1 = ((v2[1] - v0[1]) * (x - v2[0]) + (v0[0] - v2[0]) * (y - v2[1])) / area;
            float w2 = 1.0f - w0 - w1;
            if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                float z = w0 * v0[2] + w1 * v1[2] + w2 * v2[2];
                int idx = x * h + y;
                if (z < buffer[idx].z) {
                    buffer[idx].z = z; buffer[idx].active = 1;
                    buffer[idx].r = (int)(w0 * v0[3] + w1 * v1[3] + w2 * v2[3]);
                    buffer[idx].g = (int)(w0 * v0[4] + w1 * v1[4] + w2 * v2[4]);
                    buffer[idx].b = (int)(w0 * v0[5] + w1 * v1[5] + w2 * v2[5]);
                }
            }
        }
    }
}

void render_cube(Client *c) {
    int w = (c->width > 0) ? c->width : 80; int h = (c->height > 0) ? c->height : 24;
    int pw = w, ph = h * 2;
    if (c->zoom == 0.0f) c->zoom = 1.0f;
    Pixel *buffer = calloc(pw * ph, sizeof(Pixel)); if (!buffer) return;
    for(int i=0; i<pw*ph; i++) buffer[i].z = 10000.0f;
    float angle = c->angle;
    float cube[8][6] = { {-1,-1,-1,255,0,0}, {1,-1,-1,0,255,0}, {1,1,-1,0,0,255}, {-1,1,-1,255,255,0}, {-1,-1,1,0,255,255}, {1,-1,1,255,0,255}, {1,1,1,255,255,255}, {-1,1,1,128,128,128} };
    int edges[12][2] = { {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7} };
    int faces[6][4] = { {0,1,2,3}, {1,5,6,2}, {5,4,7,6}, {4,0,3,7}, {3,2,6,7}, {4,5,1,0} };
    float p2d[8][6];
    float ca = cos(angle), sa = sin(angle), cb = cos(angle * 0.5), sb = sin(angle * 0.5);
    for(int i=0; i<8; i++) {
        float x = cube[i][0], y = cube[i][1], z = cube[i][2];
        float x1 = ca*x - sa*z, z1 = sa*x + ca*z; float y1 = cb*y - sb*z1, z2 = sb*y + cb*z1;
        float scale = (ph / 2.5f) * c->zoom / (z2 + 4.0f);
        p2d[i][0] = x1 * scale * 2.0f + pw / 2; p2d[i][1] = y1 * scale + ph / 2; p2d[i][2] = z2;
        p2d[i][3] = cube[i][3]; p2d[i][4] = cube[i][4]; p2d[i][5] = cube[i][5];
    }
    if (c->cube_render_mode == 1) {
        for(int i=0; i<6; i++) {
            draw_triangle(buffer, pw, ph, p2d[faces[i][0]], p2d[faces[i][1]], p2d[faces[i][2]]);
            draw_triangle(buffer, pw, ph, p2d[faces[i][0]], p2d[faces[i][2]], p2d[faces[i][3]]);
        }
    } else {
        for(int i=0; i<12; i++) draw_line_buffer(buffer, pw, ph, (int)p2d[edges[i][0]][0], (int)p2d[edges[i][0]][1], (int)p2d[edges[i][1]][0], (int)p2d[edges[i][1]][1], (int)p2d[edges[i][0]][3], (int)p2d[edges[i][0]][4], (int)p2d[edges[i][0]][5]);
    }
    send_client(c, "\033[H");
    char *line_buf = malloc(pw * 64 + 128); if (!line_buf) { free(buffer); return; }
    int last_fg_r = -1, last_fg_g = -1, last_fg_b = -1, last_bg_r = -1, last_bg_g = -1, last_bg_b = -1;
    char buf[128];
    for(int y=0; y<ph; y+=2) {
        int line_pos = 0;
        for(int x=0; x<pw; x++) {
            Pixel top = buffer[x * ph + y], bot = buffer[x * ph + (y+1)]; buf[0] = '\0';
            if (top.active && bot.active) { if (top.r != last_fg_r || top.g != last_fg_g || top.b != last_fg_b) { sprintf(buf + strlen(buf), "\033[38;2;%d;%d;%dm", top.r, top.g, top.b); last_fg_r = top.r; last_fg_g = top.g; last_fg_b = top.b; } if (bot.r != last_bg_r || bot.g != last_bg_g || bot.b != last_bg_b) { sprintf(buf + strlen(buf), "\033[48;2;%d;%d;%dm", bot.r, bot.g, bot.b); last_bg_r = bot.r; last_bg_g = bot.g; last_bg_b = bot.b; } strcat(buf, "▀"); }
            else if (top.active) { if (top.r != last_fg_r || top.g != last_fg_g || top.b != last_fg_b) { sprintf(buf + strlen(buf), "\033[38;2;%d;%d;%dm", top.r, top.g, top.b); last_fg_r = top.r; last_fg_g = top.g; last_fg_b = top.b; } if (last_bg_r != -1) { strcat(buf, "\033[49m"); last_bg_r = -1; } strcat(buf, "▀"); }
            else if (bot.active) { if (bot.r != last_fg_r || bot.g != last_fg_g || bot.b != last_fg_b) { sprintf(buf + strlen(buf), "\033[38;2;%d;%d;%dm", bot.r, bot.g, bot.b); last_fg_r = bot.r; last_fg_g = bot.g; last_fg_b = bot.b; } if (last_bg_r != -1) { strcat(buf, "\033[49m"); last_bg_r = -1; } strcat(buf, "▄"); }
            else { if (last_fg_r != -1 || last_bg_r != -1) { strcat(buf, "\033[0m"); last_fg_r = -1; last_bg_r = -1; } strcat(buf, " "); }
            int blen = strlen(buf); memcpy(line_buf + line_pos, buf, blen); line_pos += blen;
        }
        if (last_fg_r != -1 || last_bg_r != -1) { int rlen = sprintf(line_buf + line_pos, "\033[0m"); line_pos += rlen; last_fg_r = -1; last_bg_r = -1; }
        strcpy(line_buf + line_pos, "\r\n"); send_client(c, line_buf);
    }
    free(line_buf); free(buffer); c->angle += 0.07f; set_cursor(c, 1, 1); send_client(c, TXT_CUBE_HINT);
}

void handle_input_cube(Client *c, KeyCode key, char ch) {
    if (key == KEY_CHAR) { if (ch == '1') c->cube_render_mode = 0; if (ch == '2') c->cube_render_mode = 1; }
}