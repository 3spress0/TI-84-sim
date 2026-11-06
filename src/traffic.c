#include "traffic.h"
#include "city.h"

#include <stdlib.h>
#include <string.h>

static Car cars[MAX_CARS];
static uint8_t desired;          /* how many cars the roads currently justify */
static uint32_t rescan_ms;       /* time until the next busy-road count       */
static const int8_t DX[4] = {-1, 1, 0, 0};
static const int8_t DY[4] = {0, 0, -1, 1};

static bool is_road(int x, int y) {
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return false;
    return city.tiles[y * MAP_W + x].type == TILE_ROAD;
}

void traffic_reset(void) {
    memset(cars, 0, sizeof(cars));
    desired = 0;
    rescan_ms = 0;
}

const Car *traffic_cars(void) { return cars; }

uint8_t traffic_count(void) {
    uint8_t i, n = 0;
    for (i = 0; i < MAX_CARS; i++) if (cars[i].active) n++;
    return n;
}

/* Picks a heading from (x,y): prefers straight on, never reverses unless it is a dead end. */
static bool choose_heading(Car *c) {
    int8_t options[4];
    uint8_t n = 0, k;
    for (k = 0; k < 4; k++) {
        if (DX[k] == -c->dx && DY[k] == -c->dy && (c->dx || c->dy)) continue;   /* no U-turn yet */
        if (is_road(c->x + DX[k], c->y + DY[k])) options[n++] = (int8_t)k;
    }
    if (n == 0) {                                   /* dead end: turn around if possible */
        for (k = 0; k < 4; k++)
            if (DX[k] == -c->dx && DY[k] == -c->dy && is_road(c->x + DX[k], c->y + DY[k])) {
                c->dx = DX[k]; c->dy = DY[k];
                return true;
            }
        return false;
    }
    for (k = 0; k < n; k++)                          /* keep going straight 2 times in 3 */
        if (DX[options[k]] == c->dx && DY[options[k]] == c->dy && rand() % 3) {
            return true;
        }
    k = (uint8_t)options[rand() % n];
    c->dx = DX[k];
    c->dy = DY[k];
    return true;
}

static void spawn(void) {
    uint8_t slot, tries;
    Car *c = NULL;
    for (slot = 0; slot < MAX_CARS; slot++) if (!cars[slot].active) { c = &cars[slot]; break; }
    if (!c) return;
    for (tries = 0; tries < 40; tries++) {
        uint16_t i = (uint16_t)(rand() % MAP_TILES);
        if (city.tiles[i].type != TILE_ROAD || !traffic[i]) continue;   /* only roads serving buildings */
        memset(c, 0, sizeof(*c));
        c->x = (uint8_t)(i % MAP_W);
        c->y = (uint8_t)(i / MAP_W);
        c->color = (uint8_t)(rand() & 3);
        if (!choose_heading(c)) continue;
        c->t = (uint8_t)(rand() & 127);
        c->active = true;
        return;
    }
}

void traffic_update(uint32_t elapsed_ms) {
    uint8_t i, n;
    uint32_t step;
    if (elapsed_ms > 250) elapsed_ms = 250;           /* a long hitch must not teleport cars */

    if (rescan_ms <= elapsed_ms) {                    /* about once a second: how busy is it? */
        uint16_t k, busy = 0;
        for (k = 0; k < MAP_TILES; k++)
            if (city.tiles[k].type == TILE_ROAD && traffic[k]) busy++;
        desired = (uint8_t)(busy ? (busy / 3 + 2 > MAX_CARS ? MAX_CARS : busy / 3 + 2) : 0);
        rescan_ms = 1000;
    } else {
        rescan_ms -= elapsed_ms;
    }

    step = elapsed_ms * 255 / CAR_TILE_MS;
    n = 0;
    for (i = 0; i < MAX_CARS; i++) {
        Car *c = &cars[i];
        uint32_t t;
        if (!c->active) continue;
        if (!is_road(c->x, c->y) || !is_road(c->x + c->dx, c->y + c->dy)) {
            /* road was bulldozed under us: try to find a way on, else vanish */
            if (!is_road(c->x, c->y) || !choose_heading(c)) { c->active = false; continue; }
        }
        t = c->t + step;
        while (t >= 255) {
            t -= 255;
            c->x = (uint8_t)(c->x + c->dx);
            c->y = (uint8_t)(c->y + c->dy);
            if (!choose_heading(c)) { c->active = false; break; }
        }
        if (c->active) { c->t = (uint8_t)t; n++; }
    }
    if (n > desired) {                                 /* too many: retire one now and then */
        for (i = 0; i < MAX_CARS; i++) if (cars[i].active && rand() % 20 == 0) { cars[i].active = false; break; }
    } else if (n < desired && rand() % 3 == 0) {
        spawn();
    }
}
