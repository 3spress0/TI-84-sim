#include "area.h"

void area_rect(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by, AreaCellFn fn, void *ctx) {
    int x0 = ax < bx ? ax : bx, x1 = ax < bx ? bx : ax;
    int y0 = ay < by ? ay : by, y1 = ay < by ? by : ay;
    int x, y;
    for (y = y0; y <= y1; y++)
        for (x = x0; x <= x1; x++)
            fn((uint8_t)x, (uint8_t)y, ctx);
}

void area_path(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by, bool flip,
               AreaCellFn fn, void *ctx) {
    int x = ax, y = ay;
    int sx = bx >= ax ? 1 : -1;
    int sy = by >= ay ? 1 : -1;
    if (!flip) {
        for (; x != bx; x += sx) fn((uint8_t)x, (uint8_t)y, ctx);
        for (; y != by; y += sy) fn((uint8_t)x, (uint8_t)y, ctx);
    } else {
        for (; y != by; y += sy) fn((uint8_t)x, (uint8_t)y, ctx);
        for (; x != bx; x += sx) fn((uint8_t)x, (uint8_t)y, ctx);
    }
    fn((uint8_t)bx, (uint8_t)by, ctx);   /* the far end, once */
}
