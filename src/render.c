#include "render.h"
#include "area.h"
#include "util.h"
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

static int cam_x, cam_y;   /* set each frame, used by the overlay helpers */

static void diamond(int sx, int sy) {
    gfx_Line(sx + 8, sy, sx + 15, sy + 3);
    gfx_Line(sx + 15, sy + 3, sx + 8, sy + 7);
    gfx_Line(sx + 8, sy + 7, sx, sy + 3);
    gfx_Line(sx, sy + 3, sx + 8, sy);
}

static void cell_outline(uint8_t x, uint8_t y, void *ctx) {
    (void)ctx;
    diamond(cam_x + ((int)x - (int)y) * 8, cam_y + ((int)x + (int)y) * 4);
}

/* Outer iso outline of the box between two corners. */
static void box_outline(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by) {
    int x0 = ax < bx ? ax : bx, x1 = ax < bx ? bx : ax;
    int y0 = ay < by ? ay : by, y1 = ay < by ? by : ay;
    int tx = cam_x + (x0 - y0) * 8 + 8,  ty = cam_y + (x0 + y0) * 4;
    int rx = cam_x + (x1 - y0) * 8 + 15, ry = cam_y + (x1 + y0) * 4 + 3;
    int bxp = cam_x + (x1 - y1) * 8 + 8, byp = cam_y + (x1 + y1) * 4 + 7;
    int lx = cam_x + (x0 - y1) * 8,      ly = cam_y + (x0 + y1) * 4 + 3;
    gfx_Line(tx, ty, rx, ry);
    gfx_Line(rx, ry, bxp, byp);
    gfx_Line(bxp, byp, lx, ly);
    gfx_Line(lx, ly, tx, ty);
}

static uint16_t abs_diff(uint8_t a, uint8_t b) {
    return a > b ? (uint16_t)(a - b) : (uint16_t)(b - a);
}

static void banner(int y, uint8_t color, const char *s) {
    int w = (int)gfx_GetStringWidth(s) + 6;
    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(2, y, w, 10);
    text(color, 5, y + 1, s);
}

static void draw_area_banner(const UiState *ui) {
    char b[28];
    char *p = b;
    uint16_t w = abs_diff(ui->area_x, ui->cursor_x);
    uint16_t h = abs_diff(ui->area_y, ui->cursor_y);
    if (ui->area_path) {
        p = u_str(p, "LINE ");
        p = u_uint(p, w + h + 1);
        u_str(p, " TILES");
    } else {
        p = u_str(p, "AREA ");
        p = u_uint(p, w + 1);
        p = u_str(p, "X");
        u_uint(p, h + 1);
    }
    banner(22, C_YELLOW, b);
    banner(33, C_WHITE, "2ND BUILD  ALPHA CLEAR");
    banner(44, C_GRAY, ui->area_path ? "STAT FLIP  CLEAR CANCEL" : "CLEAR CANCEL");
}

static void draw_inspector(const UiState *ui, int x) {
    const Tile *tile = &city.tiles[map_index(ui->cursor_x, ui->cursor_y)];
    char b[16];
    char *p = b;
    bool zone = tile->type >= TILE_RESIDENTIAL && tile->type <= TILE_INDUSTRIAL;
    p = u_str(p, tile_short_name((TileType)tile->type));
    if (zone) {
        if (tile->level) {
            p = u_str(p, " L");
            u_uint(p, tile->level);
        } else {
            u_str(p, " zone");
        }
    }
    text(C_WHITE, x, 224, b);
    if (zone && !city_road_access(ui->cursor_x, ui->cursor_y)) {
        text(C_RED, x + (int)gfx_GetStringWidth(b) + 6, 224, "NO ROAD");
    }
}

static void draw_hud(const UiState *ui) {
    char hot[2];
    int x;

    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(2, 2, 216, 18);
    text(C_WHITE, 5, 3, city_rank());
    x = 5 + (int)gfx_GetStringWidth(city_rank()) + 8;
    text(C_WHITE, x, 3, "POP");
    gfx_PrintUInt(stats.population, 1);
    text(C_YELLOW, 5, 12, "P");
    gfx_PrintUInt(stats.powered_pct, 1);
    text(C_CYAN, 48, 12, "W");
    gfx_PrintUInt(stats.watered_pct, 1);
    text(C_GREEN, 92, 12, "S");
    gfx_PrintUInt(stats.sewage_pct, 1);
    text(C_WHITE, 136, 12, "H");
    gfx_PrintUInt(stats.happiness, 1);
    text(C_GRAY, 172, 12, "Y");
    gfx_PrintUInt(city.ticks / 12 + 1, 1);
    gfx_PrintString("M");
    gfx_PrintUInt(city.ticks % 12 + 1, 1);

    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(0, VIEW_H, 320, 16);
    hot[0] = tile_hotkey(ui->tool);
    hot[1] = 0;
    text(C_YELLOW, 3, 224, hot);
    text(C_WHITE, 13, 224, tile_name(ui->tool));
    draw_inspector(ui, 138);

    x = 316 - (int)gfx_GetStringWidth("PAUSE 63,63");
    text(C_YELLOW, x, 224, ui->speed ? "RUN" : "PAUSE");
    if (ui->speed) gfx_PrintUInt(ui->speed, 1);
    gfx_PrintString(" ");
    gfx_SetTextFGColor(C_GRAY);
    gfx_PrintUInt(ui->cursor_x, 1);
    gfx_PrintString(",");
    gfx_PrintUInt(ui->cursor_y, 1);

    if (ui->area_active) draw_area_banner(ui);
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

void render_frame(const UiState *ui, uint16_t animation, const char *message) {
    int camera_x = 136 - ((int)ui->cursor_x - (int)ui->cursor_y) * 8;
    int camera_y = 92 - ((int)ui->cursor_x + (int)ui->cursor_y) * 4;
    int sum, x;
    cam_x = camera_x;
    cam_y = camera_y;
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

    if (ui->area_active) {
        gfx_SetColor(C_CYAN);
        if (ui->area_path) {
            area_path(ui->area_x, ui->area_y, ui->cursor_x, ui->cursor_y,
                      ui->area_flip, cell_outline, NULL);
        } else {
            box_outline(ui->area_x, ui->area_y, ui->cursor_x, ui->cursor_y);
        }
    }

    gfx_SetColor(C_YELLOW);
    diamond(camera_x + ((int)ui->cursor_x - (int)ui->cursor_y) * 8,
            camera_y + ((int)ui->cursor_x + (int)ui->cursor_y) * 4);

    draw_sidebar(ui->tool, ui->speed);
    draw_hud(ui);
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
    static const char *items[4] = {"NEW CITY", "CONTINUE", "HELP", "EXIT"};
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
    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(0, 168, 320, 14);
    text(C_WHITE, 61, 171, "BUILD WITHOUT LIMITS. SHAPE A CITY.");
    gfx_SetColor(C_NAVY);
    gfx_FillRectangle(0, 224, 320, 16);
    text(C_WHITE, 95, 228, "2ND SELECT   ARROWS MOVE");
    text(C_CYAN, 3, 228, METROCE_VERSION);
    gfx_SwapDraw();
}

void render_tool_menu(uint8_t selection) {
    uint8_t i;
    menu_box("CONSTRUCTION - UNLIMITED FUNDS");
    for (i = 1; i < TILE_COUNT; i++) {
        uint8_t col = (i - 1) / 6;
        uint8_t row = (i - 1) % 6;
        uint8_t x = 31 + col * 135;
        uint8_t y = 49 + row * 21;
        if (i == selection) {
            gfx_SetColor(C_BLUE);
            gfx_FillRectangle(x - 2, y - 3, 128, 14);
        }
        {
            char hot[2];
            hot[0] = tile_hotkey((TileType)i);
            hot[1] = 0;
            text(C_YELLOW, x, y, hot);
        }
        text(i == selection ? C_YELLOW : C_WHITE, x + 10, y, tile_name((TileType)i));
    }
    gfx_SetColor(C_BLUE);
    gfx_FillRectangle(30, 180, 260, 14);
    text(C_WHITE, 34, 183, tile_description((TileType)selection));
    text(C_GRAY, 36, 199, "ARROWS/NUMBER PICK  2ND OK  CLEAR BACK");
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

typedef struct {
    const char *key;    /* NULL for plain text lines */
    const char *text;
} HelpRow;

typedef struct {
    const char *title;
    uint8_t rows;
    const HelpRow *row;
} HelpPage;

static const HelpRow help_p0[] = {
    {"ARROWS",  "MOVE CURSOR"},
    {"2 ARROWS", "MOVE ALONG THE GRID"},
    {"HOLD",    "SPEEDS UP"},
    {"2ND",     "BUILD (HOLD: PAINT)"},
    {"ALPHA",   "BULLDOZE (HOLD: PAINT)"},
    {"STAT",    "AREA / LINE TOOL"},
    {"DEL",     "UNDO / REDO"},
    {"+  -",    "NEXT / PREVIOUS TOOL"},
    {"0-9 .",   "PICK TOOL (SEE Y=)"},
};
static const HelpRow help_p1[] = {
    {"Y=",     "TOOL MENU"},
    {"WINDOW", "TAXES + FUNDING"},
    {"ENTER",  "CITY DATA"},
    {"TRACE",  "PAUSE / RESUME"},
    {"GRAPH",  "SIMULATION SPEED"},
    {"ZOOM",   "SAVE"},
    {"MODE",   "THIS HELP"},
    {"CLEAR",  "EXIT (PRESS TWICE)"},
};
static const HelpRow help_p2[] = {
    {NULL, "STAT: MARK A CORNER, THEN MOVE."},
    {NULL, "ROAD TOOL: 2ND DRAWS AN L-SHAPED"},
    {NULL, "ROAD. STAT AGAIN FLIPS THE L."},
    {NULL, "OTHER TOOLS: 2ND FILLS THE BOX."},
    {NULL, "ALPHA BULLDOZES THE SAME SHAPE."},
    {NULL, "CLEAR CANCELS."},
    {NULL, "DEL UNDOES YOUR LAST STROKE."},
    {NULL, "DEL AGAIN REDOES IT."},
    {NULL, "ONLY THAT STROKE'S TILES CHANGE."},
};
static const HelpRow help_p3[] = {
    {NULL, "1  ROAD: HOLD 2ND AND MOVE."},
    {NULL, "2  ZONE RES, COM, IND NEXT TO A"},
    {NULL, "   ROAD. CORNERS DO NOT COUNT."},
    {NULL, "3  ADD POWER, WATER AND SEWAGE."},
    {NULL, "4  GRAPH = FASTER, TRACE = PAUSE."},
    {NULL, "5  WATCH POP AND H (HAPPINESS)."},
    {NULL, "   ENTER SHOWS DEMAND."},
};
static const HelpRow help_p4[] = {
    {NULL, "ZONES GROW WHEN DEMAND IS HIGH."},
    {NULL, "RES: HAPPINESS AND SPARE JOBS."},
    {NULL, "COM / IND: MORE POPULATION AND"},
    {NULL, "FULL POWER RAISE THEM."},
    {NULL, "HIGHER TAXES LOWER DEMAND."},
    {NULL, "MONEY IS UNLIMITED, SO TAXES"},
    {NULL, "ONLY SHAPE DEMAND AND MOOD."},
};
static const HelpRow help_p5[] = {
    {NULL, "EACH PLANT ADDS 180 UNITS."},
    {NULL, "FUNDING % SCALES THAT (WINDOW)."},
    {NULL, "LOAD: HOME L1/L2/L3 USES 1/2/3,"},
    {NULL, "COM 2/3/4, IND POWER 2/4/6."},
    {NULL, "NO WIRES OR PIPES NEEDED."},
    {NULL, "NEW GROWTH NEEDS 60%+ OF ALL 3."},
    {NULL, "UNDER 35% POWER OR WATER, OR NO"},
    {NULL, "ROAD: BUILDINGS DOWNGRADE."},
};
static const HelpRow help_p6[] = {
    {NULL, "AVERAGE OF ALL HOMES, 0-100:"},
    {NULL, "PARK +9    SCHOOL +7"},
    {NULL, "POLICE +5  FIRE +4"},
    {NULL, "POLLUTION -16 (POWER, INDUSTRY)"},
    {NULL, "NO ROAD -30"},
    {NULL, "SHORTAGES, UNEMPLOYMENT AND"},
    {NULL, "HIGH RES TAX LOWER IT."},
    {NULL, "SERVICES COVER 7-10 TILES."},
};
static const HelpRow help_p7[] = {
    {NULL, "BUILDING LEVELS 1 TO 3."},
    {NULL, "HOMES HOLD 10 / 31 / 66 PEOPLE."},
    {NULL, "JOBS: COM 7/14/21  IND 10/20/30"},
    {NULL, "HALF OF POPULATION WORKS."},
    {NULL, "RANK: 100 VILLAGE, 500 TOWN,"},
    {NULL, "2000 CITY, 6000 CAPITAL,"},
    {NULL, "15000 METROPOLIS."},
    {NULL, "CONGESTION IS DISPLAY ONLY."},
};

#define PAGE(title, rows) {title, (uint8_t)(sizeof(rows) / sizeof(rows[0])), rows}
static const HelpPage help_pages[HELP_PAGES] = {
    PAGE("CONTROLS", help_p0), PAGE("CONTROLS", help_p1), PAGE("AREA + UNDO", help_p2),
    PAGE("START HERE", help_p3), PAGE("DEMAND", help_p4), PAGE("UTILITIES", help_p5),
    PAGE("HAPPINESS", help_p6), PAGE("GROWTH + LIMITS", help_p7),
};

void render_help(uint8_t page) {
    const HelpPage *hp;
    char title[32];
    char *p = title;
    uint8_t i;
    if (page >= HELP_PAGES) page = 0;
    hp = &help_pages[page];
    p = u_str(p, hp->title);
    p = u_str(p, "  ");
    p = u_uint(p, page + 1);
    p = u_str(p, "/");
    u_uint(p, HELP_PAGES);
    gfx_FillScreen(C_NAVY);
    menu_box(title);
    for (i = 0; i < hp->rows; i++) {
        int y = 48 + i * 15;
        if (hp->row[i].key) {
            text(C_YELLOW, 34, y, hp->row[i].key);
            text(C_WHITE, 104, y, hp->row[i].text);
        } else {
            text(C_WHITE, 34, y, hp->row[i].text);
        }
    }
    text(C_GRAY, 46, 202, "LEFT/RIGHT PAGE    CLEAR BACK");
    gfx_SwapDraw();
}
