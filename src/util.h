#ifndef METROCE_UTIL_H
#define METROCE_UTIL_H

#include <stdint.h>

/* Tiny string builders so we never pull in printf. Both return the new end. */
static inline char *u_str(char *p, const char *s) {
    while (*s) *p++ = *s++;
    *p = 0;
    return p;
}

static inline char *u_uint(char *p, uint32_t v) {
    char t[11];
    uint8_t n = 0;
    do {
        t[n++] = (char)('0' + v % 10);
        v /= 10;
    } while (v);
    while (n) *p++ = t[--n];
    *p = 0;
    return p;
}

#endif
