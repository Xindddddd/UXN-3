/*
Copyright (c) 2021 Devine Lu Linvega

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.
*/

/* clang-format off */

#define PEEK2(d) (*(d) << 8 | (d)[1])
#define POKE2(d, v) { *(d) = (v) >> 8; (d)[1] = (v); }

/* clang-format on */

#define STEP_MAX 0x80000000
#define PAGE_PROGRAM 0x0100
#define PAGE_SIZE 0x10000

typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef signed short Sint16;
typedef unsigned int Uint32;

typedef struct {
	Uint8 dat[0x100], ptr;
} Stack;

typedef struct Uxn {
	Uint8 *ram, dev[0x100];
	Stack wst, rst;
} Uxn;

extern Uint8 emu_dei(Uint8 addr);
extern void emu_deo( Uint8 addr, Uint8 value);
extern Uxn uxn;

int uxn_eval(Uint16 pc);
