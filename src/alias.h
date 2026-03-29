#ifndef ALIAS_H
#define ALIAS_H

#include <stddef.h>
#include <stdint.h>

typedef unsigned char byte;
typedef uint16_t u16;
typedef uint32_t u32;
typedef size_t usize;
typedef float f32;

#define countof(x) (sizeof(x) / sizeof(x[0]))

#endif
