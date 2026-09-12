#ifndef STD_TYPES_H
#define STD_TYPES_H

#include <stdint.h>
#include <stddef.h>

/* Standard signed integer types */
typedef signed char        s_char;
typedef short              s_short;
typedef int                s_int;
typedef long               s_long;
typedef long long          s_long_long;

/* Standard unsigned integer types */
typedef unsigned char      u_char;
typedef unsigned short     u_short;
typedef unsigned int       u_int;
typedef unsigned long      u_long;
typedef unsigned long long u_long_long;

/* Fixed-width integer types, when provided */
typedef int8_t   s8;
typedef uint8_t  u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;

/* Pointer-sized unsigned type */
typedef size_t usize;

#endif