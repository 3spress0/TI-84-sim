#include "city.h"
#include "render.h"

#include <keypadc.h>
#include <stdlib.h>
#include <sys/util.h>
#include <sys/timers.h>
#include <sys/rtc.h>

static void wait_release(void) {
    do {
        kb_Scan();
        delay(25);
    } while (kb_AnyKey());
}

static void help_screen(void) {
    render_help();
    wait_release();
    for (;;) {
        kb_Scan();
        if (kb_IsDown(kb_KeyClear) || kb_IsDown(kb_Key2nd) || kb_IsDown(kb_KeyEnter)) break;
        delay(30);
    }
    wait_release();
}

/* Returns 0 for new, 1 for continue, and 2 for exit. */
static uint8_t title_screen(void) {
    bool has_save = city_has_save();
    uint8_t selection = has_save ? 1 : 0;
    wait_release();
    for (;;) {
        render_title(selection, has_save);
        kb_Scan();
        if (kb_IsDown(kb_KeyUp)) {
            selection = (selection + 3) % 4;
            wait_release();
        } else if (kb_IsDown(kb_KeyDown)) {
            selection = (selection + 1) % 4;
            wait_release();
        } else if (kb_IsDown(kb_Key2nd) || kb_IsDown(kb_KeyEnter)) {
            if (selection == 1 && !has_save) {
                wait_release();
                continue;
            }
            if (selection == 2) {
                help_screen();
                continue;
            }
            wait_release();
            return selection == 3 ? 2 : selection;
        } else if (kb_IsDown(kb_KeyClear)) {
            wait_release();
            return 2;
        }
        delay(35);
    }
}

static TileType construction_menu(TileType current) {
    uint8_t selection = current == TILE_EMPTY ? TILE_ROAD : current;
    wait_release();
    for (;;) {
        render_tool_menu(selection);
        kb_Scan();
        if (kb_IsDown(kb_KeyUp) && selection > 1) {
            selection--;
            wait_release();
        } else if (kb_IsDown(kb_KeyDown) && selection < TILE_COUNT - 1) {
            selection++;
            wait_release();
        } else if (kb_IsDown(kb_KeyLeft) && selection > 6) {
            selection -= 6;
            wait_release();
        } else if (kb_IsDown(kb_KeyRight) && selection <= 6) {
            selection += 6;
            wait_release();
        } else if (kb_IsDown(kb_Key2nd) || kb_IsDown(kb_KeyEnter)) {
            wait_release();
            return (TileType)selection;
        } else if (kb_IsDown(kb_KeyClear)) {
            wait_release();
            return current;
        }
        delay(35);
    }
}

static void budget_menu(void) {
    uint8_t selection = 0;
    wait_release();
    for (;;) {
        uint8_t *value = selection == 0 ? &city.res_tax : selection == 1 ? &city.com_tax :
                         selection == 2 ? &city.ind_tax : &city.service_funding;
        render_budget(selection);
        kb_Scan();
        if (kb_IsDown(kb_KeyUp)) {
            selection = (selection + 3) % 4;
            wait_release();
        } else if (kb_IsDown(kb_KeyDown)) {
            selection = (selection + 1) % 4;
            wait_release();
        } else if (kb_IsDown(kb_KeyLeft)) {
            uint8_t minimum = selection == 3 ? 50 : 0;
            uint8_t step = selection == 3 ? 5 : 1;
            if (*value >= minimum + step) *value -= step;
            wait_release();
        } else if (kb_IsDown(kb_KeyRight)) {
            uint8_t maximum = selection == 3 ? 150 : 20;
            uint8_t step = selection == 3 ? 5 : 1;
            if (*value <= maximum - step) *value += step;
            wait_release();
        } else if (kb_IsDown(kb_KeyClear) || kb_IsDown(kb_Key2nd)) {
            wait_release();
            city_simulate();
            return;
        }
        delay(35);
    }
}

static void stats_screen(void) {
    wait_release();
    for (;;) {
        render_stats();
        kb_Scan();
        if (kb_IsDown(kb_KeyClear) || kb_IsDown(kb_Key2nd) || kb_IsDown(kb_KeyEnter)) break;
        delay(35);
    }
    wait_release();
}

int main(void) {
    uint8_t cursor_x = 32;
    uint8_t cursor_y = 32;
    uint8_t speed = 1;
    uint8_t choice;
    uint16_t animation = 0;
    uint16_t message_timer = 0;
    const char *message = NULL;
    TileType selected = TILE_ROAD;
    static const uint8_t intervals[4] = {0, 40, 18, 7};

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

    for (;;) {
        kb_Scan();

        if (kb_IsDown(kb_KeyLeft) && cursor_x > 0) cursor_x--;
        if (kb_IsDown(kb_KeyRight) && cursor_x < MAP_W - 1) cursor_x++;
        if (kb_IsDown(kb_KeyUp) && cursor_y > 0) cursor_y--;
        if (kb_IsDown(kb_KeyDown) && cursor_y < MAP_H - 1) cursor_y++;
        if (kb_IsDown(kb_Key2nd)) city_build(cursor_x, cursor_y, selected);
        if (kb_IsDown(kb_KeyAlpha)) city_bulldoze(cursor_x, cursor_y);

        if (kb_IsDown(kb_KeyYequ)) {
            selected = construction_menu(selected);
        } else if (kb_IsDown(kb_KeyWindow)) {
            budget_menu();
        } else if (kb_IsDown(kb_KeyZoom)) {
            message = city_save() ? "CITY SAVED TO METROSAV" : "SAVE FAILED - CHECK AVAILABLE RAM";
            message_timer = 70;
        } else if (kb_IsDown(kb_KeyTrace)) {
            speed = speed ? 0 : 1;
            wait_release();
        } else if (kb_IsDown(kb_KeyGraph)) {
            speed = speed == 3 ? 1 : speed + 1;
            wait_release();
        } else if (kb_IsDown(kb_KeyEnter)) {
            stats_screen();
        } else if (kb_IsDown(kb_KeyMode)) {
            help_screen();
        } else if (kb_IsDown(kb_KeyClear)) {
            city_save();
            wait_release();
            break;
        }

        if (speed && animation % intervals[speed] == 0) city_simulate();
        render_frame(cursor_x, cursor_y, selected, speed, animation, message);
        animation++;
        if (message_timer && --message_timer == 0) message = NULL;
        delay(55);
    }

    render_end();
    return 0;
}
