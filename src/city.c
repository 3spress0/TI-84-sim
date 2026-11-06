#include "city.h"
#include "area.h"
#include "util.h"

#include <fileioc.h>
#include <stdlib.h>
#include <string.h>

#define CITY_MAGIC 0x4D455452u
#define CITY_VERSION 1
#define MAX_LEVEL 3

City city;
CityStats stats;
uint8_t coverage[MAP_TILES];
uint8_t traffic[MAP_TILES];

/* ---- one-level undo (kept outside City so the save format is unchanged) ---- */
static Tile undo_tiles[MAP_TILES];
static uint8_t undo_mask[MAP_TILES / 8];
static bool undo_pending;   /* stroke started, snapshot not taken yet        */
static bool undo_open;      /* a snapshot exists and the mask is being filled */
static bool undo_undone;    /* currently showing the "before" state           */

static void mark_dirty(uint16_t i);
static void mark_dirty_around(uint16_t i);
static void update_goal_stats(void);
static void maybe_fire(void);

static void undo_reset(void) {
    undo_pending = false;
    undo_open = false;
    undo_undone = false;
}

/* Must be called BEFORE the tile at index i is modified. */
static void undo_touch(uint16_t i) {
    if (undo_pending) {
        memcpy(undo_tiles, city.tiles, sizeof(undo_tiles));
        memset(undo_mask, 0, sizeof(undo_mask));
        undo_pending = false;
        undo_open = true;
        undo_undone = false;
    }
    if (undo_open) undo_mask[i >> 3] |= (uint8_t)(1u << (i & 7));
}

void city_undo_begin(void) {
    undo_pending = true;
}

uint8_t city_undo_toggle(void) {
    uint16_t i;
    if (!undo_open) return 0;
    for (i = 0; i < MAP_TILES; i++) {
        if (undo_mask[i >> 3] & (1u << (i & 7))) {
            Tile t = city.tiles[i];
            city.tiles[i] = undo_tiles[i];
            undo_tiles[i] = t;
            mark_dirty_around(i);
        }
    }
    undo_pending = false;
    undo_undone = !undo_undone;
    return undo_undone ? 1 : 2;
}

static uint16_t index_of(uint8_t x, uint8_t y) {
    return (uint16_t)y * MAP_W + x;
}

static uint8_t clamp_int(int value) {
    if (value < 0) return 0;
    if (value > 100) return 100;
    return (uint8_t)value;
}

static bool is_zone(TileType type) {
    return type == TILE_RESIDENTIAL || type == TILE_COMMERCIAL || type == TILE_INDUSTRIAL;
}

bool city_road_access(uint8_t x, uint8_t y) {
    if (x > 0 && city.tiles[index_of(x - 1, y)].type == TILE_ROAD) return true;
    if (x < MAP_W - 1 && city.tiles[index_of(x + 1, y)].type == TILE_ROAD) return true;
    if (y > 0 && city.tiles[index_of(x, y - 1)].type == TILE_ROAD) return true;
    if (y < MAP_H - 1 && city.tiles[index_of(x, y + 1)].type == TILE_ROAD) return true;
    return false;
}

static void add_road_traffic(uint8_t x, uint8_t y, uint8_t amount) {
    uint16_t i;
    if (x > 0) {
        i = index_of(x - 1, y);
        if (city.tiles[i].type == TILE_ROAD && traffic[i] < 255 - amount) traffic[i] += amount;
    }
    if (x < MAP_W - 1) {
        i = index_of(x + 1, y);
        if (city.tiles[i].type == TILE_ROAD && traffic[i] < 255 - amount) traffic[i] += amount;
    }
    if (y > 0) {
        i = index_of(x, y - 1);
        if (city.tiles[i].type == TILE_ROAD && traffic[i] < 255 - amount) traffic[i] += amount;
    }
    if (y < MAP_H - 1) {
        i = index_of(x, y + 1);
        if (city.tiles[i].type == TILE_ROAD && traffic[i] < 255 - amount) traffic[i] += amount;
    }
}

static void paint_coverage(uint8_t x, uint8_t y, uint8_t radius, uint8_t flag) {
    int dx, dy;
    for (dy = -(int)radius; dy <= (int)radius; dy++) {
        for (dx = -(int)radius; dx <= (int)radius; dx++) {
            int tx = (int)x + dx;
            int ty = (int)y + dy;
            if (tx < 0 || ty < 0 || tx >= MAP_W || ty >= MAP_H) continue;
            if (abs(dx) + abs(dy) > radius) continue;
            coverage[index_of((uint8_t)tx, (uint8_t)ty)] |= flag;
        }
    }
}

static void build_coverage(void) {
    uint8_t x, y;
    memset(coverage, 0, sizeof(coverage));
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            const Tile *tile = &city.tiles[index_of(x, y)];
            switch (tile->type) {
                case TILE_PARK: paint_coverage(x, y, 7, COVERAGE_PARK); break;
                case TILE_POLICE: paint_coverage(x, y, 10, COVERAGE_POLICE); break;
                case TILE_FIRE: paint_coverage(x, y, 10, COVERAGE_FIRE); break;
                case TILE_SCHOOL: paint_coverage(x, y, 9, COVERAGE_SCHOOL); break;
                case TILE_POWER: paint_coverage(x, y, 5, COVERAGE_POLLUTION); break;
                case TILE_INDUSTRIAL:
                    if (tile->level) paint_coverage(x, y, 3 + tile->level, COVERAGE_POLLUTION);
                    break;
                default: break;
            }
        }
    }
}

static uint8_t utility_percent(uint32_t capacity, uint32_t use) {
    if (!use) return 100;
    if (!capacity) return 0;
    if (capacity >= use) return 100;
    return (uint8_t)((capacity * 100) / use);
}

void city_new(void) {
    uint16_t i;
    memset(&city, 0, sizeof(city));
    undo_reset();
    city.magic = CITY_MAGIC;
    city.version = CITY_VERSION;
    city.res_tax = 9;
    city.com_tax = 9;
    city.ind_tax = 9;
    city.service_funding = 100;
    for (i = 0; i < MAP_TILES; i++) city.tiles[i].variation = (uint8_t)rand();
    memset(traffic, 0, sizeof(traffic));
    memset(coverage, 0, sizeof(coverage));
    city_simulate();
    city_goals_sync();
}

/* ---- dirty tracking so the renderer only redraws what changed ---- */
uint8_t city_dirty[MAP_TILES / 8];
bool city_dirty_flag;
uint16_t city_revision;

static void mark_dirty(uint16_t i) {
    city_dirty[i >> 3] |= (uint8_t)(1u << (i & 7));
    city_dirty_flag = true;
    city_revision++;
}

/* The tile and its four neighbours: road sprites depend on adjacent roads. */
static void mark_dirty_around(uint16_t i) {
    uint8_t x = (uint8_t)(i % MAP_W), y = (uint8_t)(i / MAP_W);
    mark_dirty(i);
    if (x > 0) mark_dirty(i - 1);
    if (x < MAP_W - 1) mark_dirty(i + 1);
    if (y > 0) mark_dirty(i - MAP_W);
    if (y < MAP_H - 1) mark_dirty(i + MAP_W);
}

bool city_build(uint8_t x, uint8_t y, TileType type) {
    Tile *tile = &city.tiles[index_of(x, y)];
    if (type <= TILE_EMPTY || type >= TILE_COUNT || tile->type != TILE_EMPTY) return false;
    undo_touch(index_of(x, y));
    tile->type = (uint8_t)type;
    tile->level = is_zone(type) ? 0 : 1;
    tile->age = 0;
    mark_dirty_around(index_of(x, y));
    return true;
}

bool city_bulldoze(uint8_t x, uint8_t y) {
    Tile *tile = &city.tiles[index_of(x, y)];
    if (tile->type == TILE_EMPTY) return false;
    undo_touch(index_of(x, y));
    memset(tile, 0, sizeof(*tile));
    tile->variation = (uint8_t)rand();
    traffic[index_of(x, y)] = 0;
    mark_dirty_around(index_of(x, y));
    return true;
}

typedef struct {
    TileType type;      /* TILE_EMPTY means bulldoze */
    uint16_t changed;
} ShapeCtx;

static void shape_cell(uint8_t x, uint8_t y, void *ctx) {
    ShapeCtx *s = (ShapeCtx *)ctx;
    bool ok = s->type == TILE_EMPTY ? city_bulldoze(x, y) : city_build(x, y, s->type);
    if (ok) s->changed++;
}

uint16_t city_apply_shape(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by,
                          bool path, bool flip, TileType type) {
    ShapeCtx s;
    s.type = type;
    s.changed = 0;
    if (path) area_path(ax, ay, bx, by, flip, shape_cell, &s);
    else area_rect(ax, ay, bx, by, shape_cell, &s);
    return s.changed;
}

/* ---- goals (derived from the live city, not saved) ---- */
typedef struct {
    uint16_t roads, l3, services;      /* services: bit per park/police/fire/school */
} GoalStats;
static GoalStats gs;
static uint16_t goals_reported;        /* goals already announced this session */

static const char *const goal_names[GOAL_COUNT] = {
    "50 CITIZENS", "250 CITIZENS", "1000 CITIZENS", "3000 CITIZENS", "6000 CITIZENS",
    "40 ROAD TILES", "HAPPY: H80 AT 200+", "EVERYONE EMPLOYED (300+)",
    "ALL 4 SERVICES (150+)", "10 LEVEL-3 BUILDINGS"
};

static void update_goal_stats(void) {
    uint16_t i;
    memset(&gs, 0, sizeof(gs));
    for (i = 0; i < MAP_TILES; i++) {
        const Tile *t = &city.tiles[i];
        if (t->type == TILE_ROAD) gs.roads++;
        else if (t->type == TILE_PARK) gs.services |= 1;
        else if (t->type == TILE_POLICE) gs.services |= 2;
        else if (t->type == TILE_FIRE) gs.services |= 4;
        else if (t->type == TILE_SCHOOL) gs.services |= 8;
        else if (is_zone(t->type) && t->level >= MAX_LEVEL) gs.l3++;
    }
}

const char *city_goal_name(uint8_t i) { return i < GOAL_COUNT ? goal_names[i] : "?"; }

void city_goal_progress(uint8_t i, uint16_t *cur, uint16_t *target) {
    static const uint16_t pops[5] = {50, 250, 1000, 3000, 6000};
    uint16_t pop = (uint16_t)(stats.population > 65535 ? 65535 : stats.population);
    *cur = 0; *target = 1;
    if (i < 5) { *cur = pop; *target = pops[i]; }
    else if (i == 5) { *cur = gs.roads; *target = 40; }
    else if (i == 6) { *cur = pop >= 200 ? stats.happiness : 0; *target = 80; }
    else if (i == 7) { *cur = (pop >= 300 && stats.jobs >= stats.population / 2) ? 1 : 0; *target = 1; }
    else if (i == 8) {
        uint8_t b, n = 0;
        for (b = 0; b < 4; b++) if (gs.services & (1u << b)) n++;
        *cur = pop >= 150 ? n : 0; *target = 4;
    } else if (i == 9) { *cur = gs.l3; *target = 10; }
}

bool city_goal_done(uint8_t i) {
    uint16_t cur, target;
    city_goal_progress(i, &cur, &target);
    return cur >= target;
}

static uint16_t goals_done_mask(void) {
    uint16_t m = 0;
    uint8_t i;
    for (i = 0; i < GOAL_COUNT; i++) if (city_goal_done(i)) m |= (uint16_t)(1u << i);
    return m;
}

/* Index of a goal that was completed since the last call, or -1. */
int8_t city_goal_new(void) {
    uint16_t fresh = goals_done_mask() & (uint16_t)~goals_reported;
    uint8_t i;
    for (i = 0; i < GOAL_COUNT; i++) {
        if (fresh & (1u << i)) { goals_reported |= (uint16_t)(1u << i); return (int8_t)i; }
    }
    return -1;
}

void city_goals_sync(void) {           /* after load/new: do not announce what is already true */
    update_goal_stats();
    goals_reported = goals_done_mask();
}

/* ---- fires: services finally have a job ---- */
static char event_text[32];
static bool event_pending;

static void maybe_fire(void) {
    uint8_t tries;
    if (stats.population < 60 || rand() % 22) return;
    for (tries = 0; tries < 60; tries++) {
        uint16_t i = (uint16_t)(rand() % MAP_TILES);
        Tile *t = &city.tiles[i];
        char *p;
        uint8_t x = (uint8_t)(i % MAP_W), y = (uint8_t)(i / MAP_W);
        if (!is_zone(t->type) || !t->level) continue;
        if (coverage[i] & COVERAGE_FIRE) {
            p = u_str(event_text, "FIRE PUT OUT AT ");
        } else {
            t->level = 0;                    /* burned down: the empty zone remains */
            t->age = 0;
            mark_dirty(i);
            p = u_str(event_text, "FIRE AT ");
        }
        p = u_uint(p, x);
        p = u_str(p, ",");
        p = u_uint(p, y);
        if (!(coverage[i] & COVERAGE_FIRE)) u_str(p, " - NO COVER");
        event_pending = true;
        return;
    }
}

const char *city_take_event(void) {
    if (!event_pending) return NULL;
    event_pending = false;
    return event_text;
}

void city_simulate(void) {
    uint8_t x, y;
    uint32_t happiness_sum = 0;
    uint32_t happy_homes = 0;
    uint32_t road_total = 0;
    uint32_t road_amount = 0;
    uint32_t workforce;
    int unemployment = 0;
    int base_happiness;

    memset(&stats, 0, sizeof(stats));
    memset(traffic, 0, sizeof(traffic));
    build_coverage();

    /* Population, employment, utility load, and local road traffic. */
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            uint16_t i = index_of(x, y);
            Tile *tile = &city.tiles[i];
            uint8_t level = tile->level;
            if (tile->age < 255) tile->age++;
            switch (tile->type) {
                case TILE_RESIDENTIAL:
                    if (level) {
                        stats.population += (uint32_t)(level * level * 7 + 3);
                        stats.buildings++;
                        stats.power_use += level;
                        stats.water_use += level;
                        stats.sewage_use += level;
                        add_road_traffic(x, y, level * 11);
                    }
                    break;
                case TILE_COMMERCIAL:
                    if (level) {
                        stats.jobs += level * 7;
                        stats.buildings++;
                        stats.power_use += level + 1;
                        stats.water_use += level;
                        stats.sewage_use += level;
                        add_road_traffic(x, y, level * 14);
                    }
                    break;
                case TILE_INDUSTRIAL:
                    if (level) {
                        stats.jobs += level * 10;
                        stats.buildings++;
                        stats.power_use += level * 2;
                        stats.water_use += level;
                        stats.sewage_use += level * 2;
                        add_road_traffic(x, y, level * 17);
                    }
                    break;
                case TILE_POWER: stats.power_capacity += 180; break;
                case TILE_WATER: stats.water_capacity += 180; break;
                case TILE_SEWAGE: stats.sewage_capacity += 180; break;
                default: break;
            }
        }
    }

    stats.power_capacity = stats.power_capacity * city.service_funding / 100;
    stats.water_capacity = stats.water_capacity * city.service_funding / 100;
    stats.sewage_capacity = stats.sewage_capacity * city.service_funding / 100;
    stats.powered_pct = utility_percent(stats.power_capacity, stats.power_use);
    stats.watered_pct = utility_percent(stats.water_capacity, stats.water_use);
    stats.sewage_pct = utility_percent(stats.sewage_capacity, stats.sewage_use);

    workforce = stats.population / 2;
    if (workforce > stats.jobs) {
        unemployment = (int)((workforce - stats.jobs) * 100 / workforce);
        if (unemployment > 100) unemployment = 100;
    }

    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            uint16_t i = index_of(x, y);
            const Tile *tile = &city.tiles[i];
            int local = 70;
            if (tile->type == TILE_ROAD && traffic[i]) {
                road_total += traffic[i] > 100 ? 100 : traffic[i];
                road_amount++;
            }
            if (tile->type != TILE_RESIDENTIAL || !tile->level) continue;
            if (coverage[i] & COVERAGE_PARK) local += 9;
            if (coverage[i] & COVERAGE_POLICE) local += 5;
            if (coverage[i] & COVERAGE_FIRE) local += 4;
            if (coverage[i] & COVERAGE_SCHOOL) local += 7;
            if (coverage[i] & COVERAGE_POLLUTION) local -= 16;
            local -= ((int)city.res_tax - 9) * 2;
            local += ((int)city.service_funding - 100) / 12;
            local -= unemployment / 5;
            if (stats.powered_pct < 100) local -= (100 - stats.powered_pct) / 3;
            if (stats.watered_pct < 100) local -= (100 - stats.watered_pct) / 3;
            if (stats.sewage_pct < 100) local -= (100 - stats.sewage_pct) / 4;
            if (!city_road_access(x, y)) local -= 30;
            {   /* a congested street outside the door is unpleasant */
                uint8_t worst = 0, k;
                int nx[4] = {(int)x - 1, (int)x + 1, (int)x, (int)x};
                int ny[4] = {(int)y, (int)y, (int)y - 1, (int)y + 1};
                for (k = 0; k < 4; k++) {
                    uint16_t ni;
                    if (nx[k] < 0 || ny[k] < 0 || nx[k] >= MAP_W || ny[k] >= MAP_H) continue;
                    ni = index_of((uint8_t)nx[k], (uint8_t)ny[k]);
                    if (city.tiles[ni].type == TILE_ROAD && traffic[ni] > worst) worst = traffic[ni];
                }
                if (worst > 90) local -= 10;
                else if (worst > 60) local -= 5;
            }
            happiness_sum += clamp_int(local);
            happy_homes++;
        }
    }

    base_happiness = happy_homes ? (int)(happiness_sum / happy_homes) : 70;
    stats.happiness = clamp_int(base_happiness);
    stats.traffic = road_amount ? (uint8_t)(road_total / road_amount) : 0;

    stats.res_demand = clamp_int(35 + stats.happiness / 3 - city.res_tax + (stats.jobs > workforce ? 14 : 0));
    stats.com_demand = clamp_int(25 + stats.population / 35 - city.com_tax + (stats.powered_pct == 100 ? 8 : -20));
    stats.ind_demand = clamp_int(48 + stats.population / 80 - city.ind_tax + (stats.powered_pct == 100 ? 8 : -20));

    /* Growth and abandonment. Empty zoned land needs road access and utilities. */
    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            Tile *tile = &city.tiles[index_of(x, y)];
            uint8_t demand;
            int chance;
            if (!is_zone(tile->type)) continue;
            if (tile->type == TILE_RESIDENTIAL) demand = stats.res_demand;
            else if (tile->type == TILE_COMMERCIAL) demand = stats.com_demand;
            else demand = stats.ind_demand;

            if (!tile->level) {
                if (city_road_access(x, y) && stats.powered_pct >= 60 && stats.watered_pct >= 60 &&
                    stats.sewage_pct >= 60 && rand() % 100 < demand / 5) {
                    tile->level = 1;
                    tile->age = 0;
                    mark_dirty(index_of(x, y));
                }
            } else if (stats.powered_pct < 35 || stats.watered_pct < 35 || !city_road_access(x, y)) {
                if (rand() % 100 < 18) {
                    tile->level--;
                    tile->age = 0;
                    mark_dirty(index_of(x, y));
                }
            } else if (tile->level < MAX_LEVEL && tile->age > 5 && demand > 45) {
                chance = demand / 18 + (stats.happiness > 75 ? 2 : 0);
                if (rand() % 100 < chance) {
                    tile->level++;
                    tile->age = 0;
                    mark_dirty(index_of(x, y));
                }
            }
        }
    }
    city.ticks++;
    update_goal_stats();
    maybe_fire();
}

const char *city_rank(void) {
    if (stats.population >= 15000) return "Metropolis";
    if (stats.population >= 6000) return "Capital";
    if (stats.population >= 2000) return "City";
    if (stats.population >= 500) return "Town";
    if (stats.population >= 100) return "Village";
    return "Settlement";
}

const char *tile_name(TileType type) {
    static const char *names[TILE_COUNT] = {
        "Grassland", "Road", "Residential", "Commercial", "Industrial", "Power plant",
        "Water tower", "Sewage plant", "Park", "Police station", "Fire station", "School"
    };
    return type < TILE_COUNT ? names[type] : "Unknown";
}

const char *tile_short_name(TileType type) {
    static const char *names[TILE_COUNT] = {
        "Grass", "Road", "Res", "Com", "Ind", "Power",
        "Water", "Sewage", "Park", "Police", "Fire", "School"
    };
    return type < TILE_COUNT ? names[type] : "?";
}

/* Number-key shortcut for each tool: 1-9, 0, then the decimal point. */
char tile_hotkey(TileType type) {
    static const char keys[] = "?1234567890.";
    return type < TILE_COUNT ? keys[type] : '?';
}

const char *tile_description(TileType type) {
    static const char *descriptions[TILE_COUNT] = {
        "Open land", "Connect zones and carry traffic", "Homes for your citizens",
        "Shops and offices", "Jobs, with pollution", "Supplies electricity",
        "Supplies fresh water", "Treats city wastewater", "Raises nearby happiness",
        "Improves safety coverage", "Provides fire coverage", "Improves education"
    };
    return type < TILE_COUNT ? descriptions[type] : "";
}

bool city_save(void) {
    uint8_t slot = ti_Open(SAVE_NAME, "w");
    bool ok;
    if (!slot) return false;
    ok = ti_Write(&city, sizeof(city), 1, slot) == 1;
    ti_Close(slot);
    return ok;
}

bool city_has_save(void) {
    uint32_t magic = 0;
    uint16_t version = 0;
    uint8_t slot = ti_Open(SAVE_NAME, "r");
    bool ok;
    if (!slot) return false;
    ok = ti_Read(&magic, sizeof(magic), 1, slot) == 1 &&
         ti_Read(&version, sizeof(version), 1, slot) == 1;
    ti_Close(slot);
    return ok && magic == CITY_MAGIC && version == CITY_VERSION;
}

bool city_load(void) {
    uint8_t slot = ti_Open(SAVE_NAME, "r");
    bool ok;
    if (!slot) return false;
    ok = ti_Read(&city, sizeof(city), 1, slot) == 1;
    ti_Close(slot);
    if (!ok || city.magic != CITY_MAGIC || city.version != CITY_VERSION) return false;
    undo_reset();
    city_simulate();
    city_goals_sync();
    return true;
}
