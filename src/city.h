#ifndef METROCE_CITY_H
#define METROCE_CITY_H

#include <stdbool.h>
#include <stdint.h>

#define MAP_W 64
#define MAP_H 64
#define MAP_TILES (MAP_W * MAP_H)
#define SAVE_NAME "METROSAV"
#define METROCE_VERSION "V1.3.0"

#define COVERAGE_PARK      0x01
#define COVERAGE_POLICE    0x02
#define COVERAGE_FIRE      0x04
#define COVERAGE_SCHOOL    0x08
#define COVERAGE_POLLUTION 0x10

typedef enum {
    TILE_EMPTY = 0,
    TILE_ROAD,
    TILE_RESIDENTIAL,
    TILE_COMMERCIAL,
    TILE_INDUSTRIAL,
    TILE_POWER,
    TILE_WATER,
    TILE_SEWAGE,
    TILE_PARK,
    TILE_POLICE,
    TILE_FIRE,
    TILE_SCHOOL,
    TILE_COUNT
} TileType;

typedef struct {
    uint8_t type;
    uint8_t level;
    uint8_t age;
    uint8_t variation;
} Tile;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint8_t res_tax;
    uint8_t com_tax;
    uint8_t ind_tax;
    uint8_t service_funding;
    uint16_t reserved;
    uint32_t ticks;
    Tile tiles[MAP_TILES];
} City;

typedef struct {
    uint32_t population;
    uint32_t jobs;
    uint32_t buildings;
    uint32_t power_use;
    uint32_t power_capacity;
    uint32_t water_use;
    uint32_t water_capacity;
    uint32_t sewage_use;
    uint32_t sewage_capacity;
    uint8_t res_demand;
    uint8_t com_demand;
    uint8_t ind_demand;
    uint8_t happiness;
    uint8_t traffic;
    uint8_t powered_pct;
    uint8_t watered_pct;
    uint8_t sewage_pct;
} CityStats;

extern City city;
extern CityStats stats;
extern uint8_t coverage[MAP_TILES];
extern uint8_t traffic[MAP_TILES];

void city_new(void);
void city_simulate(void);
bool city_build(uint8_t x, uint8_t y, TileType type);
bool city_bulldoze(uint8_t x, uint8_t y);
bool city_road_access(uint8_t x, uint8_t y);

/* Applies a tool to an L-shaped path (path=true) or a filled box (path=false).
 * type == TILE_EMPTY bulldozes. Returns how many tiles actually changed. */
uint16_t city_apply_shape(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by,
                          bool path, bool flip, TileType type);

/* One-level undo. Call city_undo_begin() at the start of every stroke; the
 * snapshot is taken lazily on the first real change, so a click that changes
 * nothing never overwrites your previous undo. Only tiles touched by the
 * stroke are restored, so growth elsewhere is left alone.
 * city_undo_toggle(): 0 = nothing to undo, 1 = undone, 2 = redone. */
/* Renderer bookkeeping: one bit per tile changed since the renderer last looked,
 * and a counter that increases on every change (the minimap cache keys on it). */
extern uint8_t city_dirty[MAP_TILES / 8];
extern bool city_dirty_flag;
extern uint16_t city_revision;

#define GOAL_COUNT 10
const char *city_goal_name(uint8_t i);
void city_goal_progress(uint8_t i, uint16_t *cur, uint16_t *target);
bool city_goal_done(uint8_t i);
int8_t city_goal_new(void);             /* newly completed goal index, or -1 */
void city_goals_sync(void);             /* call after new/load */
const char *city_take_event(void);      /* fire message once, or NULL */

void city_undo_begin(void);
uint8_t city_undo_toggle(void);
bool city_save(void);
bool city_load(void);
bool city_has_save(void);
const char *city_rank(void);
const char *tile_name(TileType type);
const char *tile_short_name(TileType type);
char tile_hotkey(TileType type);
const char *tile_description(TileType type);

#endif
