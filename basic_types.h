 /*
  File: basic_types.h
  Date: 13 December 2021
  Creator: Alexandru Filip
  Notice: (C) Copyright 2021 by Alexandru Filip. All rights reserved.
*/

#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

// --- 

typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef u32 b32;

typedef s32 int_size;
typedef int64_t int_word;
typedef uint64_t uint_word;

typedef float  r32;
typedef double r64;

struct string {
    s64   Length;
    char* Contents;

    SubscriptOperator(char);
};

#endif
