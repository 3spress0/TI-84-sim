#ifndef METROCE_RENDER_H
#define METROCE_RENDER_H

#include "city.h"
#include <stdint.h>

#define VIEW_W 288
#define VIEW_H 224
#define TILE_W 16
#define TILE_H 8
#define HELP_PAGES 8

/* Everything the renderer needs to know about the player's UI state. */
typedef struct {
    uint8_t cursor_x, cursor_y;
    TileType tool;
    uint8_t speed;
    bool area_active;      /* area/line mode is on                              */
    bool area_path;        /* true: L-shaped road line, false: filled box       */
    bool area_flip;        /* L orientation                                     */
    uint8_t area_x, area_y;/* the anchored corner                               */
} UiState;

void render_init(void);
void render_end(void);
/* Call after anything else has drawn over the screen (menus, help). */
void render_invalidate(void);
void render_frame(const UiState *ui, uint16_t animation, const char *message);
void render_title(uint8_t selection, bool has_save);
void render_tool_menu(uint8_t selection);
void render_budget(uint8_t selection);
void render_stats(void);
void render_help(uint8_t page);

#endif
