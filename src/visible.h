#ifndef METROCE_VISIBLE_H
#define METROCE_VISIBLE_H

#include <stdint.h>

#define MAP_W_VIS 64
#define MAP_H_VIS 64
#define TILE_PX_W 16
#define TILE_PX_H 8
#define MAX_SPRITE_H 52      /* tallest sprite is 51 px; tiles draw bottom-aligned */

typedef void (*TileFn)(uint8_t x, uint8_t y, int sx, int sy, void *ctx);

/*
 * Calls fn for every tile whose footprint (its 16x8 diamond box plus a sprite
 * up to MAX_SPRITE_H tall above it) can touch the screen rectangle
 * [x0,x1) x [y0,y1), in painter's order: far to near (x+y ascending, then x).
 * It may include a few tiles that do not actually touch; it never skips one
 * that does. (sx,sy) is the tile's screen position for the given camera.
 */
void visible_tiles(int cam_x, int cam_y, int x0, int y0, int x1, int y1,
                   TileFn fn, void *ctx);

#endif
