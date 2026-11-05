#include "visible.h"

static int fdiv(int a, int b) {          /* floor division, b > 0 */
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

void visible_tiles(int cam_x, int cam_y, int x0, int y0, int x1, int y1,
                   TileFn fn, void *ctx) {
    int s, s_lo, s_hi, dlo, dhi;

    /* Row sum s = x + y puts a tile at screen y = cam_y + 4s. */
    s_lo = fdiv(y0 - TILE_PX_H - cam_y, 4) + 1;                    /* bottom edge below the top */
    s_hi = fdiv(y1 + MAX_SPRITE_H - TILE_PX_H - 1 - cam_y, 4);     /* sprite top above the bottom */
    if (s_lo < 0) s_lo = 0;
    if (s_hi > MAP_W_VIS + MAP_H_VIS - 2) s_hi = MAP_W_VIS + MAP_H_VIS - 2;
    /* d = x - y puts a tile at screen x = cam_x + 8d. */
    dlo = fdiv(x0 - TILE_PX_W - cam_x, 8) + 1;
    dhi = fdiv(x1 - 1 - cam_x, 8);

    for (s = s_lo; s <= s_hi; s++) {
        int xmin = fdiv(s + dlo + 1, 2);                           /* ceil((s + dlo) / 2) */
        int xmax = fdiv(s + dhi, 2);
        int x, sx, sy;
        if (xmin < s - (MAP_H_VIS - 1)) xmin = s - (MAP_H_VIS - 1);
        if (xmin < 0) xmin = 0;
        if (xmax > s) xmax = s;
        if (xmax > MAP_W_VIS - 1) xmax = MAP_W_VIS - 1;
        if (xmin > xmax) continue;
        sx = cam_x + (2 * xmin - s) * 8;
        sy = cam_y + s * 4;
        for (x = xmin; x <= xmax; x++, sx += TILE_PX_W)
            fn((uint8_t)x, (uint8_t)(s - x), sx, sy, ctx);
    }
}
