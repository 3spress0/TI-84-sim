#include "render.h"
#include "gfx/gfx.h"

#include <graphx.h>
#include <stdio.h>

/* Fixed entries in the generated global palette. */
enum { C_CLEAR, C_WHITE, C_NAVY, C_BLUE, C_GRAY, C_DARK, C_GREEN,
       C_YELLOW, C_RED, C_CYAN, C_BLACK, C_TAN };

static gfx_sprite_t *roads[16] = {
    road0, road1, road2, road3, road4, road5, road6, road7,
    road8, road9, road10, road11, road12, road13, road14, road15
};
static gfx_sprite_t *grasses[4] = { grass0, grass1, grass2, grass3 };
static gfx_sprite_t *trees[3] = { tree0, tree1, tree2 };
static gfx_sprite_t *residential[3][2] = {
    {res1_0, res1_1}, {res2_0, res2_1}, {res3_0, res3_1}
};
static gfx_sprite_t *commercial[3][2] = {
    {com1_0, com1_1}, {com2_0, com2_1}, {com3_0, com3_1}
};
static gfx_sprite_t *industrial[3][2] = {
    {ind1_0, ind1_1}, {ind2_0, ind2_1}, {ind3_0, ind3_1}
};

static uint16_t map_index(uint8_t x, uint8_t y) {
    return (uint16_t)y * MAP_W + x;
}

static void text(uint8_t color, int x, int y, const char *string) {
    gfx_SetTextFGColor(color);
    gfx_SetTextXY(x, y);
    gfx_PrintString(string);
}

static void sprite_at_bottom(gfx_sprite_t *sprite, int x, int base_y) {
    gfx_TransparentSprite(sprite, x, base_y + TILE_H - sprite->height);
}

static uint8_t road_mask(uint8_t x, uint8_t y) {
    uint8_t mask = 0;
    if (x > 0 && city.tiles[map_index(x - 1, y)].type == TILE_ROAD) mask |= 1;
    if (x < MAP_W - 1 && city.tiles[map_index(x + 1, y)].type == TILE_ROAD) mask |= 2;
    if (y > 0 && city.tiles[map_index(x, y - 1)].type == TILE_ROAD) mask |= 4;
    if (y < MAP_H - 1 && city.tiles[map_index(x, y + 1)].type == TILE_ROAD) mask |= 8;
    return mask;
}

static void draw_tile(uint8_t x, uint8_t y, int sx, int sy, uint16_t animation) {
    uint16_t i = map_index(x, y);
    const Tile *tile = &city.tiles[i];
    uint8_t variant = tile->variation & 1;
    gfx_sprite_t *sprite = NULL;

    if (tile->type == TILE_ROAD) {
        gfx_TransparentSprite(roads[road_mask(x, y)], sx, sy);
        if (traffic[i] > 18 && ((x * 7 + y * 11 + (animation >> 3)) & 7) < 2) {
            gfx_SetColor((x + y) & 1 ? C_YELLOW : C_WHITE);
            gfx_FillRectangle(sx + 6 + ((animation >> 2) & 3), sy + 3, 2, 1);
        }
        return;
    }

    gfx_TransparentSprite(grasses[tile->variation & 3], sx, sy);
    switch (tile->type) {
        case TILE_EMPTY:
            if (tile->variation % 9 < 2) sprite = trees[tile->variation % 3];
            break;
        case TILE_RESIDENTIAL:
            if (!tile->level) sprite = zone_res;
            else sprite = residential[tile->level - 1][variant];
            break;
        case TILE_COMMERCIAL:
            if (!tile->level) sprite = zone_com;
            else sprite = commercial[tile->level - 1][variant];
            break;
        case TILE_INDUSTRIAL:
            if (!tile->level) sprite = zone_ind;
            else sprite = industrial[tile->level - 1][variant];
            break;
        case TILE_POWER: sprite = power; break;
        case TILE_WATER: sprite = water; break;
        case TILE_SEWAGE: sprite = sewage; break;
        case TILE_PARK: sprite = park; break;
        case TILE_POLICE: sprite = police; break;
        case TILE_FIRE: sprite = fire; break;
        case TILE_SCHOOL: sprite = school; break;
        default: break;
    }
    if (sprite) sprite_at_bottom(sprite, sx, sy);
}

static void draw_minimap(void) {
    uint8_t x, y;
    gfx_SetColor(C_BLACK);
    gfx_FillRectangle(288, 188, 32, 34);
    for (y = 0; y < MAP_H; y += 2) {
        for (x = 0; x < MAP_W; x += 2) {
            const Tile *tile = &city.tiles[map_index(x, y)];
            uint8_t color = C_GREEN;
            if (tile->type == TILE_ROAD) color = C_GRAY;
            else if (tile->type == TILE_RESIDENTIAL) color = C_WHITE;
            else if (tile->type == TILE_COMMERCIAL) color = C_CYAN;
            else if (tile->type == TILE_INDUSTRIAL) color = C_YELLOW;
            else if (tile->type != TILE_EMPTY) color = C_RED;
            gfx_SetColor(color);
            gfx_SetPixel(288 + x / 2, 190 + y / 2);
        }
    }
}

static void sidebar_button(uint8_t index, const char *label, uint8_t color, bool selected) {
    uint8_t y = index * 28;
    gfx_SetColor(selected ? C_BLUE : C_DARK);
    gfx_FillRectangle(290, y + 2, 28, 25);
    gfx_SetColor(color);
    gfx_FillRectangle(292, y + 4, 24, 12);
    gfx_SetColor(C_NAVY);
    switch (index) {
        case 0:
            gfx_FillRectangle(293, y + 9, 22, 3);
            gfx_SetColor(C_WHITE);
            gfx_FillRectangle(299, y + 10, 3, 1);
            gfx_FillRectangle(308, y + 10, 3, 1);
            break;
        case 1:
            gfx_FillRectangle(295, y + 7, 5, 7);
            gfx_FillRectangle(302, y + 5, 5, 9);
            gfx_FillRectangle(309, y + 8, 4, 6);
            break;
        case 2:
            gfx_SetColor(C_YELLOW);
            gfx_FillTriangle(303, y + 4, 297, y + 12, 303, y + 11);
            gfx_FillTriangle(307, y + 7, 302, y + 15, 311, y + 11);
            break;
        case 3:
            gfx_SetColor(C_WHITE);
            gfx_FillRectangle(296, y + 8, 16, 5);
            gfx_FillRectangle(301, y + 5, 6, 10);
            break;
        case 4:
            text(C_NAVY, 297, y + 6, "%");
            break;
        default:
            gfx_SetColor(C_NAVY);
            gfx_FillTriangle(299, y + 5, 299, y + 14, 310, y + 9);
            break;
    }
    text(C_WHITE, 293, y + 18, label);
}

static void draw_sidebar(TileType selected, uint8_t speed) {
    uint8_t category = 0;
    if (selected >= TILE_RESIDENTIAL && selected <= TILE_INDUSTRIAL) category = 1;
    else if (selected >= TILE_POWER && selected <= TILE_SEWAGE) category = 2;
    else if (selected >= TILE_PARK && selected <= TILE_SCHOOL) category = 3;
    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(VIEW_W, 0, 32, VIEW_H);
    sidebar_button(0, "ROAD", C_GRAY, category == 0);
    sidebar_button(1, "ZONE", C_GREEN, category == 1);
    sidebar_button(2, "UTIL", C_YELLOW, category == 2);
    sidebar_button(3, "SERV", C_CYAN, category == 3);
    sidebar_button(4, "TAX", C_TAN, false);
    sidebar_button(5, speed ? "PLAY" : "PAUSE", C_GREEN, false);
    draw_minimap();
}

static void draw_hud(uint8_t cursor_x, uint8_t cursor_y, TileType selected, uint8_t speed) {
    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(2, 2, 181, 18);
    text(C_WHITE, 5, 3, city_rank());
    text(C_WHITE, 82, 3, "POP");
    gfx_PrintUInt(stats.population, 1);
    text(C_YELLOW, 5, 12, "P");
    gfx_PrintUInt(stats.powered_pct, 1);
    text(C_CYAN, 48, 12, "W");
    gfx_PrintUInt(stats.watered_pct, 1);
    text(C_GREEN, 92, 12, "S");
    gfx_PrintUInt(stats.sewage_pct, 1);
    text(C_WHITE, 136, 12, "H");
    gfx_PrintUInt(stats.happiness, 1);

    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(0, VIEW_H, 320, 16);
    text(C_WHITE, 3, 224, tile_name(selected));
    text(C_GRAY, 110, 224, "Y1M");
    gfx_PrintUInt(city.ticks % 12 + 1, 1);
    text(C_WHITE, 160, 224, "$INF");
    text(C_YELLOW, 211, 224, speed ? "RUN" : "HOLD");
    gfx_PrintUInt(speed, 1);
    text(C_GRAY, 247, 224, "X");
    gfx_PrintUInt(cursor_x, 1);
    gfx_PrintString(",");
    gfx_PrintUInt(cursor_y, 1);
}

void render_init(void) {
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    gfx_SetTransparentColor(C_CLEAR);
    gfx_SetDrawBuffer();
    gfx_SetTextScale(1, 1);
    gfx_SetTextBGColor(C_NAVY);
}

void render_end(void) {
    gfx_End();
}

void render_frame(uint8_t cursor_x, uint8_t cursor_y, TileType selected,
                  uint8_t speed, uint16_t animation, const char *message) {
    int camera_x = 136 - ((int)cursor_x - (int)cursor_y) * 8;
    int camera_y = 92 - ((int)cursor_x + (int)cursor_y) * 4;
    int sum, x;
    gfx_FillScreen(C_NAVY);

    /* Painter's algorithm: distant tiles first, near tiles and towers last. */
    for (sum = 0; sum <= MAP_W + MAP_H - 2; sum++) {
        int start = sum - MAP_H + 1;
        int end = sum < MAP_W - 1 ? sum : MAP_W - 1;
        if (start < 0) start = 0;
        for (x = start; x <= end; x++) {
            int y = sum - x;
            int sx = camera_x + (x - y) * 8;
            int sy = camera_y + (x + y) * 4;
            if (sx <= -16 || sx >= VIEW_W + 16 || sy < -8 || sy >= VIEW_H + 8) continue;
            draw_tile((uint8_t)x, (uint8_t)y, sx, sy, animation);
        }
    }

    {
        int sx = camera_x + ((int)cursor_x - (int)cursor_y) * 8;
        int sy = camera_y + ((int)cursor_x + (int)cursor_y) * 4;
        gfx_SetColor(C_YELLOW);
        gfx_Line(sx + 8, sy, sx + 15, sy + 3);
        gfx_Line(sx + 15, sy + 3, sx + 8, sy + 7);
        gfx_Line(sx + 8, sy + 7, sx, sy + 3);
        gfx_Line(sx, sy + 3, sx + 8, sy);
    }

    draw_sidebar(selected, speed);
    draw_hud(cursor_x, cursor_y, selected, speed);
    if (message) {
        gfx_SetColor(C_YELLOW);
        gfx_FillRectangle(44, 96, 200, 24);
        gfx_SetColor(C_NAVY);
        gfx_FillRectangle(46, 98, 196, 20);
        text(C_WHITE, 52, 104, message);
    }
    gfx_SwapDraw();
}

static void menu_box(const char *title) {
    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(20, 18, 280, 204);
    gfx_SetColor(C_BLUE);
    gfx_Rectangle(22, 20, 276, 200);
    text(C_YELLOW, 32, 28, title);
    gfx_HorizLine(30, 39, 260);
}

void render_title(uint8_t selection, bool has_save) {
    static const char *items[4] = {"NEW CITY", "CONTINUE", "CONTROLS", "EXIT"};
    uint8_t i;
    gfx_FillScreen(C_NAVY);
    gfx_SetColor(C_BLUE);
    gfx_FillRectangle(0, 150, 320, 90);
    for (i = 0; i < 9; i++) {
        gfx_sprite_t *sprite = i & 1 ? com2_0 : res3_0;
        sprite_at_bottom(sprite, i * 36 - 3, 185 - (i % 3) * 4);
    }
    gfx_SetTextScale(3, 3);
    text(C_WHITE, 59, 35, "METROCE");
    gfx_SetTextScale(1, 1);
    text(C_CYAN, 76, 65, "ISOMETRIC CITY SIMULATOR");
    for (i = 0; i < 4; i++) {
        if (i == selection) {
            gfx_SetColor(C_BLUE);
            gfx_FillRectangle(102, 91 + i * 15, 116, 13);
        }
        text(i == 1 && !has_save ? C_GRAY : (i == selection ? C_YELLOW : C_WHITE),
             129, 94 + i * 15, items[i]);
    }
    text(C_WHITE, 61, 173, "BUILD WITHOUT LIMITS. SHAPE A CITY.");
    text(C_WHITE, 95, 230, "2ND SELECT   ARROWS MOVE");
    gfx_SwapDraw();
}

void render_tool_menu(uint8_t selection) {
    uint8_t i;
    menu_box("CONSTRUCTION - UNLIMITED FUNDS");
    for (i = 1; i < TILE_COUNT; i++) {
        uint8_t col = (i - 1) / 6;
        uint8_t row = (i - 1) % 6;
        uint8_t x = 31 + col * 135;
        uint8_t y = 49 + row * 22;
        if (i == selection) {
            gfx_SetColor(C_BLUE);
            gfx_FillRectangle(x - 2, y - 2, 128, 20);
        }
        text(i == selection ? C_YELLOW : C_WHITE, x, y, tile_name((TileType)i));
        text(C_GRAY, x, y + 9, tile_description((TileType)i));
    }
    text(C_WHITE, 52, 199, "ARROWS SELECT   2ND CONFIRM   CLEAR BACK");
    gfx_SwapDraw();
}

void render_budget(uint8_t selection) {
    const char *labels[4] = {"RESIDENTIAL TAX", "COMMERCIAL TAX", "INDUSTRIAL TAX", "SERVICE FUNDING"};
    uint8_t values[4] = {city.res_tax, city.com_tax, city.ind_tax, city.service_funding};
    uint8_t i;
    menu_box("CITY BUDGET - SANDBOX MODE");
    text(C_GRAY, 34, 48, "TAXES CHANGE DEMAND AND HAPPINESS.");
    for (i = 0; i < 4; i++) {
        uint8_t y = 70 + i * 24;
        if (i == selection) {
            gfx_SetColor(C_BLUE);
            gfx_FillRectangle(31, y - 3, 250, 20);
        }
        text(i == selection ? C_YELLOW : C_WHITE, 38, y, labels[i]);
        gfx_PrintUInt(values[i], 1);
        gfx_PrintString("%");
        gfx_SetColor(C_GRAY);
        gfx_FillRectangle(185, y + 1, 75, 6);
        gfx_SetColor(i == 3 ? C_CYAN : C_GREEN);
        gfx_FillRectangle(185, y + 1, values[i] * 75 / (i == 3 ? 150 : 20), 6);
    }
    text(C_WHITE, 52, 190, "LEFT/RIGHT ADJUST   CLEAR DONE");
    gfx_SwapDraw();
}

void render_stats(void) {
    menu_box("CITY DATA");
    text(C_WHITE, 35, 50, "HAPPINESS"); gfx_PrintUInt(stats.happiness, 1); gfx_PrintString("%");
    text(C_WHITE, 35, 65, "POPULATION"); gfx_PrintUInt(stats.population, 1);
    text(C_WHITE, 35, 80, "JOBS"); gfx_PrintUInt(stats.jobs, 1);
    text(C_WHITE, 35, 95, "AVG CONGESTION"); gfx_PrintUInt(stats.traffic, 1); gfx_PrintString("%");
    text(C_GREEN, 35, 120, "RES DEMAND"); gfx_PrintUInt(stats.res_demand, 1);
    text(C_CYAN, 35, 135, "COM DEMAND"); gfx_PrintUInt(stats.com_demand, 1);
    text(C_YELLOW, 35, 150, "IND DEMAND"); gfx_PrintUInt(stats.ind_demand, 1);
    text(C_GRAY, 35, 174, "ZONES NEED ROADS, POWER, WATER + SEWAGE.");
    text(C_WHITE, 91, 199, "PRESS CLEAR TO RETURN");
    gfx_SwapDraw();
}

void render_help(void) {
    menu_box("CONTROLS");
    text(C_WHITE, 34, 48, "ARROWS    MOVE CURSOR / SCROLL CITY");
    text(C_WHITE, 34, 63, "2ND       BUILD SELECTED ITEM");
    text(C_WHITE, 34, 78, "ALPHA     BULLDOZE");
    text(C_WHITE, 34, 93, "Y=        CONSTRUCTION MENU");
    text(C_WHITE, 34, 108, "WINDOW    TAXES AND SERVICE FUNDING");
    text(C_WHITE, 34, 123, "ZOOM      SAVE CITY");
    text(C_WHITE, 34, 138, "TRACE     PAUSE / RESUME");
    text(C_WHITE, 34, 153, "GRAPH     CHANGE SIMULATION SPEED");
    text(C_WHITE, 34, 168, "ENTER     CITY DATA");
    text(C_WHITE, 34, 183, "CLEAR     SAVE AND EXIT");
    text(C_YELLOW, 52, 202, "MONEY IS UNLIMITED IN THIS MODE");
    gfx_SwapDraw();
}
