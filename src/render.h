#ifndef METROCE_RENDER_H
#define METROCE_RENDER_H

#include "city.h"
#include <stdint.h>

#define VIEW_W 288
#define VIEW_H 224
#define TILE_W 16
#define TILE_H 8

void render_init(void);
void render_end(void);
void render_frame(uint8_t cursor_x, uint8_t cursor_y, TileType selected,
                  uint8_t speed, uint16_t animation, const char *message);
void render_title(uint8_t selection, bool has_save);
void render_tool_menu(uint8_t selection);
void render_budget(uint8_t selection);
void render_stats(void);
void render_help(void);

#endif
