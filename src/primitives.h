#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef unsigned char byte;

typedef  uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef   int8_t  i8;
typedef  int16_t i16;
typedef  int32_t i32;

typedef  float f32;
typedef double f64;

typedef size_t usize;

#define countof(x) (sizeof(x) / sizeof(x[0]))

#define NOOP

typedef struct {
	i32 x, y;
} IVec2;

typedef struct {
	byte *data;
	usize size;
} ByteView;

#define bv(x) ((ByteView){(byte *)(x), sizeof(x)})

typedef struct {
	const char *ptr;
	usize len;
} StringView;

#define sv(x) ((StringView){x, sizeof(x) - 1})
#define SV(x) (int)(x).len, (x).ptr
#define PRISV "%.*s"

static inline bool
sv_eq(
	StringView a,
	StringView b
) {
	return a.len == b.len
	    && !memcmp(a.ptr, b.ptr, a.len);
}

/* "ArrayView" */
#define av(T, a) ((T##View){ .ptr = (a), .len = countof(a) })


#endif // PRIMITIVES_H
