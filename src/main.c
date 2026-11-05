#include "city.h"
#include "input.h"
#include "render.h"
#include "util.h"

#include <keypadc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/util.h>
#include <sys/rtc.h>
#include <sys/timers.h>
#include <time.h>

/* Milliseconds between simulation ticks at speed 1, 2 and 3 (0 = paused).
 * Estimated from the old frame-count intervals; tune to taste. */
static const uint16_t sim_ms[4] = {0, 3500, 1600, 650};

#define MESSAGE_MS 1500
#define EXIT_CONFIRM_MS 2000

/* Number-key shortcut for each tool, in TileType order starting at TILE_ROAD. */
static const kb_lkey_t tool_keys[TILE_COUNT - 1] = {
    kb_Key1, kb_Key2, kb_Key3, kb_Key4, kb_Key5, kb_Key6,
    kb_Key7, kb_Key8, kb_Key9, kb_Key0, kb_KeyDecPnt
};

/* Screen direction (index = (sy+1)*3 + (sx+1)) to map-grid step.
 * Cardinals move along both map axes (straight on screen); diagonals move
 * along one map axis (straight along the grid). */
static const int8_t map_dx[9] = {-1, -1,  0, -1, 0,  1, 0, 1, 1};
static const int8_t map_dy[9] = { 0, -1, -1,  1, 0, -1, 1, 1, 0};

static void wait_release(void) {
    do {
        kb_Scan();
        delay(20);
    } while (kb_AnyKey());
}

static bool leave_key(void) {
    return input_pressed(kb_KeyClear) || input_pressed(kb_Key2nd) || input_pressed(kb_KeyEnter);
}

/* Pages 0..HELP_PAGES-1; Left/Right flip, Clear/2nd/Enter leave. */
static void help_screen(uint8_t page) {
    wait_release();
    input_sync();
    for (;;) {
        render_help(page);
        input_scan();
        if (input_pressed(kb_KeyRight) && page + 1 < HELP_PAGES) page++;
        if (input_pressed(kb_KeyLeft) && page > 0) page--;
        if (leave_key()) break;
        delay(20);
    }
    wait_release();
    input_sync();
}

/* Returns 0 for new, 1 for continue, and 2 for exit. */
static uint8_t title_screen(void) {
    bool has_save = city_has_save();
    uint8_t selection = has_save ? 1 : 0;
    wait_release();
    input_sync();
    for (;;) {
        render_title(selection, has_save);
        input_scan();
        if (input_pressed(kb_KeyUp)) {
            selection = (selection + 3) % 4;
        } else if (input_pressed(kb_KeyDown)) {
            selection = (selection + 1) % 4;
        } else if (input_pressed(kb_Key2nd) || input_pressed(kb_KeyEnter)) {
            if (selection == 1 && !has_save) continue;
            if (selection == 2) {
                help_screen(0);
                continue;
            }
            wait_release();
            return selection == 3 ? 2 : selection;
        } else if (input_pressed(kb_KeyClear)) {
            wait_release();
            return 2;
        }
        delay(20);
    }
}

static TileType construction_menu(TileType current) {
    uint8_t selection = current == TILE_EMPTY ? TILE_ROAD : current;
    Repeater rep = {NAV_NONE, 0};
    render_snapshot_backdrop();
    wait_release();
    input_sync();
    for (;;) {
        uint8_t i;
        render_tool_menu(selection);
        input_scan();
        for (i = 0; i < TILE_COUNT - 1; i++) {      /* number keys pick directly */
            if (input_pressed(tool_keys[i])) {
                wait_release();
                return (TileType)(i + 1);
            }
        }
        if (repeat_fire(&rep, input_nav_code(), clock(), MENU_REPEAT_DELAY, MENU_REPEAT_RATE)) {
            switch (rep.last) {
                case NAV_UP:    if (selection > 1) selection--; break;
                case NAV_DOWN:  if (selection < TILE_COUNT - 1) selection++; break;
                case NAV_LEFT:  if (selection > 6) selection -= 6; break;
                case NAV_RIGHT:
                    if (selection <= 6) {
                        selection += 6;
                        if (selection > TILE_COUNT - 1) selection = TILE_COUNT - 1;
                    }
                    break;
                default: break;
            }
        }
        if (input_pressed(kb_Key2nd) || input_pressed(kb_KeyEnter)) {
            wait_release();
            return (TileType)selection;
        }
        if (input_pressed(kb_KeyClear)) {
            wait_release();
            return current;
        }
        delay(20);
    }
}

static void budget_menu(void) {
    uint8_t selection = 0;
    Repeater rep = {NAV_NONE, 0};
    render_snapshot_backdrop();
    wait_release();
    input_sync();
    for (;;) {
        uint8_t *value = selection == 0 ? &city.res_tax : selection == 1 ? &city.com_tax :
                         selection == 2 ? &city.ind_tax : &city.service_funding;
        uint8_t minimum = selection == 3 ? 50 : 0;
        uint8_t maximum = selection == 3 ? 150 : 20;
        uint8_t step = selection == 3 ? 5 : 1;
        render_budget(selection);
        input_scan();
        if (repeat_fire(&rep, input_nav_code(), clock(), MENU_REPEAT_DELAY, MENU_REPEAT_RATE / 2)) {
            switch (rep.last) {
                case NAV_UP:    selection = (selection + 3) % 4; break;
                case NAV_DOWN:  selection = (selection + 1) % 4; break;
                case NAV_LEFT:  if (*value >= minimum + step) *value -= step; break;
                case NAV_RIGHT: if (*value + step <= maximum) *value += step; break;
                default: break;
            }
        }
        if (input_pressed(kb_KeyClear) || input_pressed(kb_Key2nd)) {
            wait_release();
            city_simulate();
            return;
        }
        delay(20);
    }
}

static void stats_screen(void) {
    render_snapshot_backdrop();
    wait_release();
    input_sync();
    for (;;) {
        render_stats();
        input_scan();
        if (leave_key()) break;
        delay(20);
    }
    wait_release();
    input_sync();
}

/* ---- cursor movement ---- */

static bool step_x(UiState *ui, int8_t d) {
    int n = (int)ui->cursor_x + d;
    if (n < 0 || n >= MAP_W) return false;
    ui->cursor_x = (uint8_t)n;
    return true;
}

static bool step_y(UiState *ui, int8_t d) {
    int n = (int)ui->cursor_y + d;
    if (n < 0 || n >= MAP_H) return false;
    ui->cursor_y = (uint8_t)n;
    return true;
}

static void paint_here(const UiState *ui, bool bulldoze) {
    if (bulldoze) city_bulldoze(ui->cursor_x, ui->cursor_y);
    else city_build(ui->cursor_x, ui->cursor_y, ui->tool);
}

/*
 * Moves along the grid. A screen-direction (cardinal) arrow is worth two grid
 * axes, but taking both at once would keep x+y constant and make half the tiles
 * unreachable with single arrows. So a cardinal step takes ONE axis and
 * alternates (x, then y, then x ...): two presses travel the full distance and
 * every tile can be reached. If the chosen axis is blocked by the map edge the
 * other one is tried, so movement never stalls. Each step moves one cell, so
 * a painted trail is always 4-connected.
 */
static uint8_t cardinal_phase;

static void move_cursor(UiState *ui, int8_t sx, int8_t sy, uint8_t count,
                        bool paint, bool bulldoze) {
    uint8_t idx = (uint8_t)((sy + 1) * 3 + (sx + 1));
    int8_t dx = map_dx[idx], dy = map_dy[idx];
    uint8_t i;
    for (i = 0; i < count; i++) {
        bool moved;
        if (dx && dy) {
            if (cardinal_phase) moved = step_y(ui, dy) || step_x(ui, dx);
            else                moved = step_x(ui, dx) || step_y(ui, dy);
            cardinal_phase ^= 1;
        } else {
            moved = (dx && step_x(ui, dx)) || (dy && step_y(ui, dy));
        }
        if (moved && paint) paint_here(ui, bulldoze);
    }
}

static char msg_buf[32];

static const char *count_message(const char *verb, uint16_t n) {
    char *p = u_str(msg_buf, verb);
    p = u_uint(p, n);
    u_str(p, n == 1 ? " TILE" : " TILES");
    return msg_buf;
}

int main(void) {
    UiState ui;
    uint8_t choice;
    uint8_t resume_speed = 1;
    uint32_t last_sim, message_until = 0, exit_deadline = 0;
    const char *message = NULL;
    bool exit_armed = false;

    memset(&ui, 0, sizeof(ui));
    ui.cursor_x = 32;
    ui.cursor_y = 32;
    ui.speed = 1;
    ui.tool = TILE_ROAD;
    ui.area_path = true;

    srand(rtc_Time());
    render_init();
    choice = title_screen();
    if (choice == 2) {
        render_end();
        return 0;
    }
    if (choice == 1) {
        if (!city_load()) city_new();
    } else {
        city_new();
    }

    input_init();
    last_sim = clock();

    for (;;) {
        uint32_t now = clock();
        bool build, bulldoze, painting;
        int8_t sx, sy;
        uint8_t count, i;

        input_scan();

        /* --- tool selection: number keys and +/- --- */
        for (i = 0; i < TILE_COUNT - 1; i++) {
            if (input_pressed(tool_keys[i])) ui.tool = (TileType)(i + 1);
        }
        if (input_pressed(kb_KeyAdd)) ui.tool = ui.tool >= TILE_COUNT - 1 ? TILE_ROAD : (TileType)(ui.tool + 1);
        if (input_pressed(kb_KeySub)) ui.tool = ui.tool <= TILE_ROAD ? (TileType)(TILE_COUNT - 1) : (TileType)(ui.tool - 1);
        ui.area_path = ui.tool == TILE_ROAD;

        build = input_down(kb_Key2nd);
        bulldoze = input_down(kb_KeyAlpha);
        painting = (build || bulldoze) && !ui.area_active;

        /* --- strokes: every press of 2nd or Alpha starts a new undo step --- */
        if (input_pressed(kb_Key2nd) || input_pressed(kb_KeyAlpha)) city_undo_begin();

        /* --- area / line mode --- */
        if (input_pressed(kb_KeyStat)) {
            if (!ui.area_active) {
                ui.area_active = true;
                ui.area_x = ui.cursor_x;
                ui.area_y = ui.cursor_y;
                ui.area_flip = false;
            } else {
                ui.area_flip = !ui.area_flip;
            }
        }

        if (ui.area_active) {
            bool go = input_pressed(kb_Key2nd);
            bool clear = input_pressed(kb_KeyAlpha);
            if (go || clear) {
                uint16_t n = city_apply_shape(ui.area_x, ui.area_y, ui.cursor_x, ui.cursor_y,
                                              ui.area_path, ui.area_flip,
                                              clear ? TILE_EMPTY : ui.tool);
                message = n ? count_message(clear ? "CLEARED " : "BUILT ", n) : "NOTHING CHANGED";
                message_until = now + MS(MESSAGE_MS);
                ui.area_active = false;
            }
        } else {
            /* A single tap acts on the current tile at once. */
            if (input_pressed(kb_Key2nd) && !input_down(kb_KeyAlpha)) {
                if (!city_build(ui.cursor_x, ui.cursor_y, ui.tool) &&
                    city.tiles[(uint16_t)ui.cursor_y * MAP_W + ui.cursor_x].type != TILE_EMPTY) {
                    message = "TILE IN USE - ALPHA CLEARS";
                    message_until = now + MS(MESSAGE_MS);
                }
            }
            if (input_pressed(kb_KeyAlpha)) city_bulldoze(ui.cursor_x, ui.cursor_y);
        }

        /* --- cursor --- */
        if (input_arrows(now, painting, &sx, &sy, &count)) {
            move_cursor(&ui, sx, sy, count, painting, bulldoze);
        }

        /* --- single-press commands --- */
        if (input_pressed(kb_KeyDel)) {
            uint8_t r = city_undo_toggle();
            message = r == 0 ? "NOTHING TO UNDO" : r == 1 ? "UNDONE" : "REDONE";
            message_until = now + MS(MESSAGE_MS);
            if (r) city_simulate();
        } else if (input_pressed(kb_KeyYequ)) {
            ui.tool = construction_menu(ui.tool);
            input_sync();
            render_invalidate();
        } else if (input_pressed(kb_KeyWindow)) {
            budget_menu();
            input_sync();
            render_invalidate();
        } else if (input_pressed(kb_KeyZoom)) {
            message = city_save() ? "CITY SAVED TO METROSAV" : "SAVE FAILED - CHECK AVAILABLE RAM";
            message_until = now + MS(MESSAGE_MS);
        } else if (input_pressed(kb_KeyTrace)) {
            if (ui.speed) {
                resume_speed = ui.speed;
                ui.speed = 0;
            } else {
                ui.speed = resume_speed;
                last_sim = now;
            }
        } else if (input_pressed(kb_KeyGraph)) {
            ui.speed = ui.speed >= 3 ? 1 : (uint8_t)(ui.speed + 1);
            resume_speed = ui.speed;
            last_sim = now;
        } else if (input_pressed(kb_KeyEnter)) {
            stats_screen();
            input_sync();
            render_invalidate();
        } else if (input_pressed(kb_KeyMode)) {
            help_screen(0);
            render_invalidate();
        } else if (input_pressed(kb_KeyClear)) {
            if (ui.area_active) {
                ui.area_active = false;
                message = "AREA CANCELED";
                message_until = now + MS(MESSAGE_MS);
            } else if (exit_armed) {
                city_save();
                wait_release();
                break;
            } else {
                exit_armed = true;
                exit_deadline = now + MS(EXIT_CONFIRM_MS);
                message = "CLEAR AGAIN: SAVE AND EXIT";
                message_until = exit_deadline;
            }
        }
        if (exit_armed && time_due(now, exit_deadline)) exit_armed = false;
        if (message && time_due(now, message_until)) message = NULL;

        /* --- simulation runs on the clock, not on frames --- */
        if (ui.speed && time_due(now, last_sim + MS(sim_ms[ui.speed]))) {
            city_simulate();
            last_sim = now;
        }

        /* Traffic dots only move when tiles are redrawn, so key them to the sim clock. */
        render_frame(&ui, (uint16_t)(city.ticks << 2), message);
    }

    render_end();
    return 0;
}
