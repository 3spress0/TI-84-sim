#ifndef METROCE_INPUT_H
#define METROCE_INPUT_H

#include <keypadc.h>
#include <stdbool.h>
#include <stdint.h>

/* clock() runs at 32768 ticks per second on the CE. MS() is for constants. */
#define MS(n) ((uint32_t)(n) * 32768UL / 1000UL)

/* ---- Feel constants. All guesses until tested on hardware: tune freely. ---- */
#define ARROW_REPEAT_DELAY  MS(230)   /* hold this long before auto-repeat starts   */
#define ARROW_REPEAT_RATE   MS(45)    /* time between repeats while held            */
#define ARROW_CHANGE_DELAY  MS(120)   /* after rolling the thumb to a new direction */
#define ARROW_ACCEL2        MS(1000)  /* held this long: 2 steps per repeat         */
#define ARROW_ACCEL3        MS(2000)  /* held this long: 3 steps per repeat         */
#define MENU_REPEAT_DELAY   MS(300)
#define MENU_REPEAT_RATE    MS(80)

enum { NAV_NONE = 0, NAV_UP, NAV_DOWN, NAV_LEFT, NAV_RIGHT };

typedef struct {
    uint8_t  last;   /* last code seen, NAV_NONE when idle */
    uint32_t next;   /* when the next repeat fires         */
} Repeater;

/* Wrap-safe "has this deadline passed" for clock() values. */
static inline bool time_due(uint32_t now, uint32_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}

/* Keyboard snapshot with edge detection. Call input_scan() once per frame. */
void input_init(void);                   /* full reset + sync */
void input_sync(void);                   /* rescan and forget edges (use after menus) */
void input_scan(void);
bool input_down(kb_lkey_t key);          /* held right now                */
bool input_pressed(kb_lkey_t key);       /* went down since the last scan */

/*
 * Cursor input. Returns true when the cursor should move this frame.
 *   *sx,*sy  screen direction, each -1/0/+1 (cardinals or diagonals)
 *   *count   how many times to apply it (acceleration); always 1 if precise
 * First press waits one scan so a two-arrow diagonal press is not misread as
 * a cardinal step followed by a diagonal.
 */
bool input_arrows(uint32_t now, bool precise, int8_t *sx, int8_t *sy, uint8_t *count);

/* Menu helpers. */
uint8_t input_nav_code(void);            /* NAV_* of the arrows, up/down win ties */
bool repeat_fire(Repeater *r, uint8_t code, uint32_t now, uint32_t delay, uint32_t rate);

#endif
