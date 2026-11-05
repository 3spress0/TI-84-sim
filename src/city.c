#include "city.h"

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
}

bool city_build(uint8_t x, uint8_t y, TileType type) {
    Tile *tile = &city.tiles[index_of(x, y)];
    if (type <= TILE_EMPTY || type >= TILE_COUNT || tile->type != TILE_EMPTY) return false;
    tile->type = (uint8_t)type;
    tile->level = is_zone(type) ? 0 : 1;
    tile->age = 0;
    return true;
}

bool city_bulldoze(uint8_t x, uint8_t y) {
    Tile *tile = &city.tiles[index_of(x, y)];
    if (tile->type == TILE_EMPTY) return false;
    memset(tile, 0, sizeof(*tile));
    tile->variation = (uint8_t)rand();
    traffic[index_of(x, y)] = 0;
    return true;
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
                }
            } else if (stats.powered_pct < 35 || stats.watered_pct < 35 || !city_road_access(x, y)) {
                if (rand() % 100 < 18) {
                    tile->level--;
                    tile->age = 0;
                }
            } else if (tile->level < MAX_LEVEL && tile->age > 5 && demand > 45) {
                chance = demand / 18 + (stats.happiness > 75 ? 2 : 0);
                if (rand() % 100 < chance) {
                    tile->level++;
                    tile->age = 0;
                }
            }
        }
    }
    city.ticks++;
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
    city_simulate();
    return true;
}
