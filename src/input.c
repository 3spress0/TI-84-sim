#include "input.h"

#include <string.h>

static uint8_t cur[8];
static uint8_t prev[8];

typedef struct {
    bool     pending;      /* saw a first press, waiting one scan to settle */
    bool     active;       /* committed and (possibly) repeating            */
    int8_t   hx, hy;       /* held direction                                */
    uint32_t t_start;      /* when this direction began                     */
    uint32_t t_next;       /* next repeat time                              */
} ArrowState;

static ArrowState arrow;

static void snapshot(void) {
    uint8_t g;
    for (g = 1; g < 8; g++) cur[g] = kb_Data[g];
}

void input_sync(void) {
    kb_Scan();
    snapshot();
    memcpy(prev, cur, sizeof(prev));
    memset(&arrow, 0, sizeof(arrow));
}

void input_init(void) {
    memset(cur, 0, sizeof(cur));
    memset(prev, 0, sizeof(prev));
    input_sync();
}

void input_scan(void) {
    memcpy(prev, cur, sizeof(prev));
    kb_Scan();
    snapshot();
}

bool input_down(kb_lkey_t key) {
    return (cur[key >> 8] & (uint8_t)key) != 0;
}

bool input_pressed(kb_lkey_t key) {
    uint8_t g = (uint8_t)(key >> 8);
    uint8_t m = (uint8_t)key;
    return (cur[g] & m) && !(prev[g] & m);
}

static void read_dir(int8_t *sx, int8_t *sy) {
    int8_t x = 0, y = 0;
    if (input_down(kb_KeyRight)) x++;
    if (input_down(kb_KeyLeft))  x--;
    if (input_down(kb_KeyDown))  y++;
    if (input_down(kb_KeyUp))    y--;
    *sx = x;
    *sy = y;
}

bool input_arrows(uint32_t now, bool precise, int8_t *sx, int8_t *sy, uint8_t *count) {
    int8_t x, y;
    read_dir(&x, &y);
    *count = 1;

    if (!x && !y) {
        bool tap = arrow.pending;           /* released before it settled: still a tap */
        *sx = arrow.hx;
        *sy = arrow.hy;
        memset(&arrow, 0, sizeof(arrow));
        return tap && (*sx || *sy);
    }

    if (!arrow.active && !arrow.pending) {  /* fresh press: wait one scan */
        arrow.pending = true;
        arrow.hx = x;
        arrow.hy = y;
        arrow.t_start = now;
        return false;
    }

    if (arrow.pending) {                    /* settled: commit what is held now */
        arrow.pending = false;
        arrow.active = true;
        arrow.hx = x;
        arrow.hy = y;
        arrow.t_start = now;
        arrow.t_next = now + ARROW_REPEAT_DELAY;
        *sx = x;
        *sy = y;
        return true;
    }

    if (x != arrow.hx || y != arrow.hy) {   /* thumb rolled to another direction */
        arrow.hx = x;
        arrow.hy = y;
        arrow.t_start = now;
        arrow.t_next = now + ARROW_CHANGE_DELAY;
        *sx = x;
        *sy = y;
        return true;
    }

    if (time_due(now, arrow.t_next)) {      /* auto-repeat */
        uint32_t held = now - arrow.t_start;
        if (!precise) {
            if (held >= ARROW_ACCEL3) *count = 3;
            else if (held >= ARROW_ACCEL2) *count = 2;
        }
        arrow.t_next = now + ARROW_REPEAT_RATE;
        *sx = x;
        *sy = y;
        return true;
    }
    return false;
}

uint8_t input_nav_code(void) {
    if (input_down(kb_KeyUp))    return NAV_UP;
    if (input_down(kb_KeyDown))  return NAV_DOWN;
    if (input_down(kb_KeyLeft))  return NAV_LEFT;
    if (input_down(kb_KeyRight)) return NAV_RIGHT;
    return NAV_NONE;
}

bool repeat_fire(Repeater *r, uint8_t code, uint32_t now, uint32_t delay, uint32_t rate) {
    if (!code) {
        r->last = NAV_NONE;
        return false;
    }
    if (code != r->last) {
        r->last = code;
        r->next = now + delay;
        return true;
    }
    if (time_due(now, r->next)) {
        r->next = now + rate;
        return true;
    }
    return false;
}
