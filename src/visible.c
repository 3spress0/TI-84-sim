#include "visible.h"

static int fdiv(int a, int b) {          /* floor division, b > 0 */
    return a >= 0 ? a / b : -((-a + b - 1) / b);
}

void visible_tiles(int zs, int cam_x, int cam_y, int x0, int y0, int x1, int y1,
                   TileFn fn, void *ctx) {
    int s, s_lo, s_hi, dlo, dhi;
    int hw = 8 << zs, hh = 4 << zs, tw = TILE_PX_W << zs, th = TILE_PX_H << zs, maxh = MAX_SPRITE_H << zs;

    /* Row sum s = x + y puts a tile at screen y = cam_y + 4s. */
    s_lo = fdiv(y0 - th - cam_y, hh) + 1;                    /* bottom edge below the top */
    s_hi = fdiv(y1 + maxh - th - 1 - cam_y, hh);     /* sprite top above the bottom */
    if (s_lo < 0) s_lo = 0;
    if (s_hi > MAP_W_VIS + MAP_H_VIS - 2) s_hi = MAP_W_VIS + MAP_H_VIS - 2;
    /* d = x - y puts a tile at screen x = cam_x + 8d. */
    dlo = fdiv(x0 - tw - cam_x, hw) + 1;
    dhi = fdiv(x1 - 1 - cam_x, hw);

    for (s = s_lo; s <= s_hi; s++) {
        int xmin = fdiv(s + dlo + 1, 2);                           /* ceil((s + dlo) / 2) */
        int xmax = fdiv(s + dhi, 2);
        int x, sx, sy;
        if (xmin < s - (MAP_H_VIS - 1)) xmin = s - (MAP_H_VIS - 1);
        if (xmin < 0) xmin = 0;
        if (xmax > s) xmax = s;
        if (xmax > MAP_W_VIS - 1) xmax = MAP_W_VIS - 1;
        if (xmin > xmax) continue;
        sx = cam_x + (2 * xmin - s) * hw;
        sy = cam_y + s * hh;
        for (x = xmin; x <= xmax; x++, sx += tw)
            fn((uint8_t)x, (uint8_t)(s - x), sx, sy, ctx);
    }
}
