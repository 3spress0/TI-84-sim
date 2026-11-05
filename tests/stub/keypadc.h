/* Host stub of keypadc.h: same key encodings, kb_Data is a plain array. */
#ifndef STUB_KEYPADC_H
#define STUB_KEYPADC_H
#include <stdint.h>
typedef uint8_t kb_key_t;
typedef uint16_t kb_lkey_t;
extern uint8_t kb_Data[8];
void kb_Scan(void);
uint8_t kb_AnyKey(void);
#define kb_KeyGraph     ((kb_lkey_t)(1 << 8 | 1<<0))
#define kb_KeyTrace     ((kb_lkey_t)(1 << 8 | 1<<1))
#define kb_KeyZoom      ((kb_lkey_t)(1 << 8 | 1<<2))
#define kb_KeyWindow    ((kb_lkey_t)(1 << 8 | 1<<3))
#define kb_KeyYequ      ((kb_lkey_t)(1 << 8 | 1<<4))
#define kb_Key2nd       ((kb_lkey_t)(1 << 8 | 1<<5))
#define kb_KeyMode      ((kb_lkey_t)(1 << 8 | 1<<6))
#define kb_KeyDel       ((kb_lkey_t)(1 << 8 | 1<<7))
#define kb_KeyFenetre   kb_KeyWindow
#define kb_KeyFx        kb_KeyYequ
#define kb_KeySuppr     kb_KeyDel
#define kb_KeySto       ((kb_lkey_t)(2 << 8 | 1<<1))
#define kb_KeyLn        ((kb_lkey_t)(2 << 8 | 1<<2))
#define kb_KeyLog       ((kb_lkey_t)(2 << 8 | 1<<3))
#define kb_KeySquare    ((kb_lkey_t)(2 << 8 | 1<<4))
#define kb_KeyRecip     ((kb_lkey_t)(2 << 8 | 1<<5))
#define kb_KeyMath      ((kb_lkey_t)(2 << 8 | 1<<6))
#define kb_KeyAlpha     ((kb_lkey_t)(2 << 8 | 1<<7))
#define kb_KeyTglExact  kb_KeyRecip
#define kb_Key0         ((kb_lkey_t)(3 << 8 | 1<<0))
#define kb_Key1         ((kb_lkey_t)(3 << 8 | 1<<1))
#define kb_Key4         ((kb_lkey_t)(3 << 8 | 1<<2))
#define kb_Key7         ((kb_lkey_t)(3 << 8 | 1<<3))
#define kb_KeyComma     ((kb_lkey_t)(3 << 8 | 1<<4))
#define kb_KeySin       ((kb_lkey_t)(3 << 8 | 1<<5))
#define kb_KeyApps      ((kb_lkey_t)(3 << 8 | 1<<6))
#define kb_KeyGraphVar  ((kb_lkey_t)(3 << 8 | 1<<7))
#define kb_KeyTrig      kb_KeySin
#define kb_KeyMatrice   kb_KeyApps
#define kb_KeyDecPnt    ((kb_lkey_t)(4 << 8 | 1<<0))
#define kb_Key2         ((kb_lkey_t)(4 << 8 | 1<<1))
#define kb_Key5         ((kb_lkey_t)(4 << 8 | 1<<2))
#define kb_Key8         ((kb_lkey_t)(4 << 8 | 1<<3))
#define kb_KeyLParen    ((kb_lkey_t)(4 << 8 | 1<<4))
#define kb_KeyCos       ((kb_lkey_t)(4 << 8 | 1<<5))
#define kb_KeyPrgm      ((kb_lkey_t)(4 << 8 | 1<<6))
#define kb_KeyStat      ((kb_lkey_t)(4 << 8 | 1<<7))
#define kb_KeyResol     kb_KeyCos
#define kb_KeyChs      ((kb_lkey_t)(5 << 8 | 1<<0))
#define kb_Key3        ((kb_lkey_t)(5 << 8 | 1<<1))
#define kb_Key6        ((kb_lkey_t)(5 << 8 | 1<<2))
#define kb_Key9        ((kb_lkey_t)(5 << 8 | 1<<3))
#define kb_KeyRParen   ((kb_lkey_t)(5 << 8 | 1<<4))
#define kb_KeyTan      ((kb_lkey_t)(5 << 8 | 1<<5))
#define kb_KeyVars     ((kb_lkey_t)(5 << 8 | 1<<6))
#define kb_KeyFrac     kb_KeyTan
#define kb_KeyEnter    ((kb_lkey_t)(6 << 8 | 1<<0))
#define kb_KeyAdd      ((kb_lkey_t)(6 << 8 | 1<<1))
#define kb_KeySub      ((kb_lkey_t)(6 << 8 | 1<<2))
#define kb_KeyMul      ((kb_lkey_t)(6 << 8 | 1<<3))
#define kb_KeyDiv      ((kb_lkey_t)(6 << 8 | 1<<4))
#define kb_KeyPower    ((kb_lkey_t)(6 << 8 | 1<<5))
#define kb_KeyClear    ((kb_lkey_t)(6 << 8 | 1<<6))
#define kb_KeyAnnul    kb_KeyClear
#define kb_KeyDown     ((kb_lkey_t)(7 << 8 | 1<<0))
#define kb_KeyLeft     ((kb_lkey_t)(7 << 8 | 1<<1))
#define kb_KeyRight    ((kb_lkey_t)(7 << 8 | 1<<2))
#define kb_KeyUp       ((kb_lkey_t)(7 << 8 | 1<<3))
#define kb_KeyStore     _Pragma("GCC warning \"'kb_KeyStore' is deprecated, use 'kb_KeySto' instead\"") kb_KeySto
#define kb_KeyPgrm      _Pragma("GCC warning \"'kb_KeyPgrm' is deprecated, use 'kb_KeyPrgm' instead\"") kb_KeyPrgm
#endif
