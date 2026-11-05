/* Host-side tests for the MetroCE input, area, undo and movement logic.
 * Build/run with tests/run.sh. The calculator build itself is not run here. */
#define main metroce_main
#include "../src/main.c"
#include "../src/area.h"
#include "../src/visible.h"
#undef main

#include <assert.h>
#include <stdio.h>

uint8_t kb_Data[8];
void kb_Scan(void) {}
uint8_t kb_AnyKey(void) { uint8_t g; for (g = 1; g < 8; g++) if (kb_Data[g]) return 1; return 0; }
void delay(uint16_t ms) { (void)ms; }

/* render.h stubs so main.c links */
void render_init(void) {}
void render_invalidate(void) {}
void render_end(void) {}
void render_frame(const UiState *u, uint16_t a, const char *m) { (void)u; (void)a; (void)m; }
void render_title(uint8_t s, bool h) { (void)s; (void)h; }
void render_tool_menu(uint8_t s) { (void)s; }
void render_budget(uint8_t s) { (void)s; }
void render_stats(void) {}
void render_help(uint8_t p) { (void)p; }

static int checks;
#define CHECK(c) do { checks++; if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static void keys(kb_lkey_t a, kb_lkey_t b, kb_lkey_t c) {
    memset(kb_Data, 0, sizeof(kb_Data));
    if (a) kb_Data[a >> 8] |= (uint8_t)a;
    if (b) kb_Data[b >> 8] |= (uint8_t)b;
    if (c) kb_Data[c >> 8] |= (uint8_t)c;
}

static int step(uint32_t t, bool precise, int8_t *sx, int8_t *sy, uint8_t *n) {
    input_scan();
    return input_arrows(t, precise, sx, sy, n);
}

static int test_input(void) {
    int8_t sx, sy; uint8_t n;
    uint32_t t = MS(1000), t0;
    int fired, i;

    /* edge detection: held key fires once */
    keys(0, 0, 0); input_init();
    keys(kb_Key2nd, 0, 0); input_scan(); CHECK(input_pressed(kb_Key2nd));
    input_scan(); CHECK(!input_pressed(kb_Key2nd) && input_down(kb_Key2nd));

    /* held-from-before keys are not "pressed" after a sync */
    keys(kb_Key2nd, 0, 0); input_sync(); input_scan(); CHECK(!input_pressed(kb_Key2nd));

    /* a tap (released before settling) still moves exactly once */
    keys(0, 0, 0); input_init();
    keys(kb_KeyRight, 0, 0); CHECK(!step(t, false, &sx, &sy, &n));
    keys(0, 0, 0);           CHECK(step(t + MS(40), false, &sx, &sy, &n)); CHECK(sx == 1 && sy == 0 && n == 1);
    CHECK(!step(t + MS(80), false, &sx, &sy, &n));

    /* hold: settle, one step, delay, then repeats */
    keys(0, 0, 0); input_init();
    keys(kb_KeyDown, 0, 0); CHECK(!step(t, false, &sx, &sy, &n));
    t0 = t + MS(50);
    CHECK(step(t0, false, &sx, &sy, &n)); CHECK(sx == 0 && sy == 1 && n == 1);
    CHECK(!step(t0 + MS(100), false, &sx, &sy, &n));            /* inside repeat delay */
    CHECK(!step(t0 + MS(229), false, &sx, &sy, &n));
    CHECK(step(t0 + MS(231), false, &sx, &sy, &n)); CHECK(n == 1);
    CHECK(!step(t0 + MS(260), false, &sx, &sy, &n));            /* rate-limited */
    { uint32_t tt; int last_n = 0, ok = 1;
      for (tt = t0 + MS(276); tt <= t0 + MS(1100); tt += MS(45)) {      /* a steady frame rate */
          if (!step(tt, false, &sx, &sy, &n)) ok = 0; else last_n = n;
      }
      CHECK(ok); CHECK(last_n == 2);                                    /* accel tier 2 after 1 s */
      for (; tt <= t0 + MS(2100); tt += MS(45)) { if (step(tt, false, &sx, &sy, &n)) last_n = n; }
      CHECK(last_n == 3);                                               /* tier 3 after 2 s */
      CHECK(step(tt, true, &sx, &sy, &n)); CHECK(n == 1);               /* painting: no accel */
      tt += MS(300);                                                    /* one slow frame (a redraw) */
      CHECK(step(tt, true, &sx, &sy, &n)); CHECK(n == 1);               /* painting: no catch-up */
      tt += MS(300);
      CHECK(step(tt, false, &sx, &sy, &n)); CHECK(n == 3);             /* navigating: bounded catch-up */
    }

    /* two-arrow chord pressed a scan apart resolves to ONE diagonal step */
    keys(0, 0, 0); input_init();
    keys(kb_KeyRight, 0, 0);             CHECK(!step(t, false, &sx, &sy, &n));
    keys(kb_KeyRight, kb_KeyDown, 0);    CHECK(step(t + MS(40), false, &sx, &sy, &n));
    CHECK(sx == 1 && sy == 1 && n == 1);
    keys(0, 0, 0); CHECK(!step(t + MS(90), false, &sx, &sy, &n));

    /* rolling the thumb to a new direction steps immediately, then waits */
    keys(0, 0, 0); input_init();
    keys(kb_KeyRight, 0, 0); step(t, false, &sx, &sy, &n); step(t + MS(40), false, &sx, &sy, &n);
    keys(kb_KeyDown, 0, 0);  CHECK(step(t + MS(100), false, &sx, &sy, &n)); CHECK(sx == 0 && sy == 1);
    CHECK(!step(t + MS(150), false, &sx, &sy, &n));
    CHECK(step(t + MS(225), false, &sx, &sy, &n));

    /* opposing arrows cancel */
    keys(0, 0, 0); input_init();
    keys(kb_KeyLeft, kb_KeyRight, 0); CHECK(!step(t, false, &sx, &sy, &n)); CHECK(!step(t + MS(60), false, &sx, &sy, &n));

    /* repeat_fire */
    { Repeater r = {NAV_NONE, 0};
      CHECK(!repeat_fire(&r, NAV_NONE, t, MS(300), MS(80)));
      CHECK(repeat_fire(&r, NAV_UP, t, MS(300), MS(80)));
      CHECK(!repeat_fire(&r, NAV_UP, t + MS(200), MS(300), MS(80)));
      CHECK(repeat_fire(&r, NAV_UP, t + MS(301), MS(300), MS(80)));
      CHECK(repeat_fire(&r, NAV_DOWN, t + MS(310), MS(300), MS(80)));   /* new direction: immediate */
      repeat_fire(&r, NAV_NONE, t + MS(320), MS(300), MS(80));
      CHECK(repeat_fire(&r, NAV_DOWN, t + MS(330), MS(300), MS(80)));   /* released then pressed */
    }

    /* wrap-safe deadlines */
    CHECK(time_due(5, 0xFFFFFFF0u));
    CHECK(!time_due(0xFFFFFFF0u, 5));

    /* sanity on MS() */
    CHECK(MS(1000) == 32768UL && MS(230) == 7536UL);

    /* nothing ever fires with no arrow held */
    keys(0, 0, 0); input_init();
    fired = 0; for (i = 0; i < 50; i++) fired += step(t + MS(i * 60), false, &sx, &sy, &n);
    CHECK(fired == 0);
    return 0;
}

typedef struct { int n; uint8_t xs[256], ys[256]; } Trail;
static void trail_cb(uint8_t x, uint8_t y, void *c) { Trail *t = (Trail *)c; t->xs[t->n] = x; t->ys[t->n] = y; t->n++; }

static int test_area(void) {
    uint8_t ax, ay, bx, by;
    for (ax = 0; ax < 5; ax++) for (ay = 0; ay < 5; ay++)
    for (bx = 0; bx < 5; bx++) for (by = 0; by < 5; by++) {
        int flip, i, j;
        for (flip = 0; flip < 2; flip++) {
            Trail t; t.n = 0;
            area_path(ax, ay, bx, by, flip, trail_cb, &t);
            CHECK(t.n == abs((int)ax - bx) + abs((int)ay - by) + 1);          /* length */
            CHECK(t.xs[0] == ax && t.ys[0] == ay);                              /* starts at A */
            CHECK(t.xs[t.n - 1] == bx && t.ys[t.n - 1] == by);                  /* ends at B */
            for (i = 1; i < t.n; i++)                                           /* 4-connected */
                CHECK(abs((int)t.xs[i] - t.xs[i - 1]) + abs((int)t.ys[i] - t.ys[i - 1]) == 1);
            for (i = 0; i < t.n; i++) for (j = i + 1; j < t.n; j++)             /* no repeats */
                CHECK(!(t.xs[i] == t.xs[j] && t.ys[i] == t.ys[j]));
        }
        { Trail r; r.n = 0; area_rect(ax, ay, bx, by, trail_cb, &r);
          CHECK(r.n == (abs((int)ax - bx) + 1) * (abs((int)ay - by) + 1)); }
    }
    return 0;
}

static int test_city(void) {
    uint16_t n;
    city_new();
    CHECK(city_undo_toggle() == 0);                                  /* nothing yet */

    /* build, undo, redo */
    city_undo_begin();
    CHECK(city_build(5, 5, TILE_ROAD));
    CHECK(city.tiles[5 * MAP_W + 5].type == TILE_ROAD);
    CHECK(city_undo_toggle() == 1);
    CHECK(city.tiles[5 * MAP_W + 5].type == TILE_EMPTY);
    CHECK(city_undo_toggle() == 2);
    CHECK(city.tiles[5 * MAP_W + 5].type == TILE_ROAD);

    /* a stroke that changes nothing must not destroy the previous undo */
    city_undo_begin();
    CHECK(!city_build(5, 5, TILE_PARK));                             /* occupied */
    CHECK(city_undo_toggle() == 1);
    CHECK(city.tiles[5 * MAP_W + 5].type == TILE_EMPTY);

    /* undo touches only the stroke's tiles: growth elsewhere survives */
    city_undo_begin();
    CHECK(city_build(6, 6, TILE_RESIDENTIAL));
    city.tiles[10 * MAP_W + 10].type = TILE_COMMERCIAL;              /* "grown" elsewhere */
    city.tiles[10 * MAP_W + 10].level = 2;
    CHECK(city_undo_toggle() == 1);
    CHECK(city.tiles[6 * MAP_W + 6].type == TILE_EMPTY);
    CHECK(city.tiles[10 * MAP_W + 10].type == TILE_COMMERCIAL && city.tiles[10 * MAP_W + 10].level == 2);

    /* dirty tracking: a road marks itself and its four neighbours; revision advances */
    city_new();
    memset(city_dirty, 0, sizeof(city_dirty)); city_dirty_flag = false;
    { uint16_t rev = city_revision;
      #define DIRTY(x, y) ((city_dirty[((y) * MAP_W + (x)) >> 3] >> (((y) * MAP_W + (x)) & 7)) & 1)
      city_undo_begin();
      CHECK(city_build(20, 20, TILE_ROAD));
      CHECK(city_dirty_flag && city_revision != rev);
      CHECK(DIRTY(20, 20) && DIRTY(19, 20) && DIRTY(21, 20) && DIRTY(20, 19) && DIRTY(20, 21));
      CHECK(!DIRTY(21, 21) && !DIRTY(30, 30));
      memset(city_dirty, 0, sizeof(city_dirty)); city_dirty_flag = false;
      CHECK(city_bulldoze(20, 20));
      CHECK(city_dirty_flag && DIRTY(20, 20) && DIRTY(20, 21));
      memset(city_dirty, 0, sizeof(city_dirty)); city_dirty_flag = false;
      CHECK(!city_build(0, 0, TILE_EMPTY));                              /* failed build: nothing dirty */
      CHECK(!city_dirty_flag);
      CHECK(city_undo_toggle() == 1);                                    /* undo marks too */
      CHECK(city_dirty_flag);
    }

    /* shapes */
    city_new();
    city_undo_begin();
    n = city_apply_shape(2, 2, 5, 4, false, false, TILE_RESIDENTIAL);
    CHECK(n == 12);
    n = city_apply_shape(2, 2, 5, 4, false, false, TILE_RESIDENTIAL);   /* already built */
    CHECK(n == 0);
    n = city_apply_shape(0, 10, 6, 13, true, false, TILE_ROAD);
    CHECK(n == 6 + 3 + 1);
    CHECK(city.tiles[10 * MAP_W + 0].type == TILE_ROAD && city.tiles[13 * MAP_W + 6].type == TILE_ROAD);
    CHECK(city.tiles[13 * MAP_W + 0].type == TILE_EMPTY);                /* flip=false goes X first */
    n = city_apply_shape(2, 2, 5, 4, false, false, TILE_EMPTY);          /* bulldoze the box */
    CHECK(n == 12);
    CHECK(city_undo_toggle() == 1);                                      /* one stroke, all undone */
    CHECK(city.tiles[2 * MAP_W + 2].type == TILE_EMPTY && city.tiles[10 * MAP_W + 0].type == TILE_EMPTY);

    /* shape that is cut off by the map edge never writes out of bounds */
    n = city_apply_shape(60, 60, 63, 63, false, false, TILE_PARK);
    CHECK(n == 16);

    /* hotkeys are unique and cover every tool */
    { int i, j;
      for (i = 1; i < TILE_COUNT; i++) for (j = i + 1; j < TILE_COUNT; j++)
          CHECK(tile_hotkey((TileType)i) != tile_hotkey((TileType)j));
      CHECK(tile_hotkey(TILE_ROAD) == '1' && tile_hotkey(TILE_FIRE) == '0' && tile_hotkey(TILE_SCHOOL) == '.');
      for (i = 0; i < TILE_COUNT - 1; i++) CHECK(tile_hotkey((TileType)(i + 1)) ==
          "1234567890."[i]);
    }
    return 0;
}

static int test_move(void) {
    UiState ui;
    int i, d;
    memset(&ui, 0, sizeof(ui));
    ui.tool = TILE_ROAD;

    /* every one of the 8 screen directions, painting: trail must be 4-connected */
    for (d = 0; d < 9; d++) {
        int8_t sx = (int8_t)(d % 3 - 1), sy = (int8_t)(d / 3 - 1);
        uint8_t px, py; int roads = 0, x, y;
        if (!sx && !sy) continue;
        city_new();
        ui.cursor_x = 30; ui.cursor_y = 30;
        city_undo_begin();
        paint_here(&ui, false);
        for (i = 0; i < 6; i++) move_cursor(&ui, sx, sy, 1, true, false);
        /* collect road cells and check each (except the first) touches a neighbour edge-wise */
        for (y = 0; y < MAP_H; y++) for (x = 0; x < MAP_W; x++) if (city.tiles[y * MAP_W + x].type == TILE_ROAD) {
            int nb = 0; roads++;
            if (x > 0 && city.tiles[y * MAP_W + x - 1].type == TILE_ROAD) nb++;
            if (x < MAP_W - 1 && city.tiles[y * MAP_W + x + 1].type == TILE_ROAD) nb++;
            if (y > 0 && city.tiles[(y - 1) * MAP_W + x].type == TILE_ROAD) nb++;
            if (y < MAP_H - 1 && city.tiles[(y + 1) * MAP_W + x].type == TILE_ROAD) nb++;
            CHECK(nb >= 1);
        }
        CHECK(roads >= 7);
        (void)px; (void)py;
    }

    /* screen directions really are screen directions: Right = +16px x, Up = -8px y */
    { int8_t sx = 1, sy = 0; uint8_t idx = (uint8_t)((sy + 1) * 3 + (sx + 1));
      int mx = map_dx[idx], my = map_dy[idx];
      CHECK((mx - my) * 8 == 16 && (mx + my) * 4 == 0);                  /* Right */
      sx = 0; sy = -1; idx = (uint8_t)((sy + 1) * 3 + (sx + 1));
      mx = map_dx[idx]; my = map_dy[idx];
      CHECK((mx - my) * 8 == 0 && (mx + my) * 4 == -8);                  /* Up */
      sx = 1; sy = 1; idx = (uint8_t)((sy + 1) * 3 + (sx + 1));
      mx = map_dx[idx]; my = map_dy[idx];
      CHECK((mx == 1 && my == 0));                                       /* Right+Down = +x grid axis */
    }

    /* edges: cursor never leaves the map, even moving with count 3 */
    ui.cursor_x = 0; ui.cursor_y = 0;
    for (i = 0; i < 20; i++) move_cursor(&ui, 0, -1, 3, false, false);
    CHECK(ui.cursor_x == 0 && ui.cursor_y == 0);
    ui.cursor_x = 63; ui.cursor_y = 63;
    for (i = 0; i < 20; i++) move_cursor(&ui, 0, 1, 3, false, false);
    CHECK(ui.cursor_x == 63 && ui.cursor_y == 63);
    return 0;
}

typedef struct { int n; int s[5000], x[5000]; uint8_t seen[64][64]; } Visit;
static void visit_cb(uint8_t x, uint8_t y, int sx, int sy, void *c) {
    Visit *v = (Visit *)c;
    v->s[v->n] = x + y; v->x[v->n] = x; v->n++;
    v->seen[y][x]++;
    (void)sx; (void)sy;
}

static int test_visible(void) {
    static Visit v;
    unsigned seed = 12345;
    int trial;
    for (trial = 0; trial < 3000; trial++) {
        int cam_x, cam_y, x0, y0, x1, y1, x, y, i;
        seed = seed * 1103515245u + 12345u;
        cam_x = (int)((seed >> 8) % 1400) - 700;
        seed = seed * 1103515245u + 12345u;
        cam_y = (int)((seed >> 8) % 700) - 400;
        seed = seed * 1103515245u + 12345u;
        x0 = (int)((seed >> 8) % 280); y0 = (int)((seed >> 4) % 220);
        seed = seed * 1103515245u + 12345u;
        x1 = x0 + 1 + (int)((seed >> 8) % (320 - x0)); y1 = y0 + 1 + (int)((seed >> 4) % (224 - y0));
        memset(&v, 0, sizeof(v));
        visible_tiles(cam_x, cam_y, x0, y0, x1, y1, visit_cb, &v);
        for (i = 1; i < v.n; i++)                                       /* painter's order */
            CHECK(v.s[i] > v.s[i - 1] || (v.s[i] == v.s[i - 1] && v.x[i] > v.x[i - 1]));
        for (y = 0; y < 64; y++) for (x = 0; x < 64; x++) {
            int sx = cam_x + (x - y) * 8, sy = cam_y + (x + y) * 4;
            int bx0 = sx, bx1 = sx + 16, by0 = sy + 8 - MAX_SPRITE_H, by1 = sy + 8;
            int touches = bx1 > x0 && bx0 < x1 && by1 > y0 && by0 < y1;
            CHECK(v.seen[y][x] <= 1);                                   /* never twice */
            if (touches) CHECK(v.seen[y][x] == 1);                      /* never skipped */
        }
        CHECK(v.n < 1200);                                              /* and it really culls */
    }
    return 0;
}

int main(void) {
    if (test_visible()) return 1;
    if (test_input()) return 1;
    if (test_area())  return 1;
    if (test_city())  return 1;
    if (test_move())  return 1;
    printf("ALL TESTS PASSED (%d checks)\n", checks);
    return 0;
}
