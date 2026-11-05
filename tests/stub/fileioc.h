#ifndef STUB_FILEIOC_H
#define STUB_FILEIOC_H
#include <stddef.h>
#include <stdint.h>
static inline uint8_t ti_Open(const char *n, const char *m) { (void)n; (void)m; return 0; }
static inline size_t ti_Read(void *p, size_t s, size_t c, uint8_t h) { (void)p; (void)s; (void)h; return c; }
static inline size_t ti_Write(const void *p, size_t s, size_t c, uint8_t h) { (void)p; (void)s; (void)h; return c; }
static inline int ti_Close(uint8_t h) { (void)h; return 0; }
#endif
