#ifndef METROCE_AREA_H
#define METROCE_AREA_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*AreaCellFn)(uint8_t x, uint8_t y, void *ctx);

/* Every cell of the filled box between two corners (inclusive). */
void area_rect(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by, AreaCellFn fn, void *ctx);

/*
 * Every cell of an L-shaped, 4-connected path from A to B (inclusive).
 * flip=false walks along X first, then Y. flip=true walks Y first, then X.
 * Each cell is visited exactly once.
 */
void area_path(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by, bool flip,
               AreaCellFn fn, void *ctx);

#endif
