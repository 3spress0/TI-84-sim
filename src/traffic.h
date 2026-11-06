#ifndef METROCE_TRAFFIC_H
#define METROCE_TRAFFIC_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_CARS 20
#define CAR_TILE_MS 520          /* time to cross one tile */

typedef struct {
    uint8_t x, y;                /* tile the car is leaving   */
    int8_t dx, dy;               /* heading, one grid step    */
    uint8_t t;                   /* 0..255 progress toward the next tile */
    uint8_t color;               /* index into a small palette of car colours */
    bool active;
} Car;

void traffic_reset(void);

/* Advances every car by the time since the last call (milliseconds are derived
 * from clock ticks by the caller), spawns or removes cars to match how busy the
 * roads are, and keeps every car on a road. */
void traffic_update(uint32_t elapsed_ms);

const Car *traffic_cars(void);           /* array of MAX_CARS, check .active */
uint8_t traffic_count(void);

#endif
