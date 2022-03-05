
#ifndef COMMON_DEFS_H_
#define COMMON_DEFS_H_

#include <stdio.h>
#include <stdlib.h>

#include <stdarg.h>
#include <stdint.h>

#define internal      static
#define global        static
#define local_persist static

#define Between(Lower, Value, Upper) (((unsigned)((Value) - (Lower))) <= ((unsigned)((Upper) - (Lower))))
#define IsUpper(Char) Between('A', Char, 'Z')
#define IsLower(Char) Between('a', Char, 'z')
#define IsDigit(Char) Between('0', Char, '9')

// ---

#define Kilobytes(Size) (1024LL *     ((u64)Size))
#define Megabytes(Size) (1024LL * Kilobytes(Size))
#define Gigabytes(Size) (1024LL * Megabytes(Size))
#define Terabytes(Size) (1024LL * Terabytes(Size))


#if DEBUG

#include <signal.h>
#if defined(SIGTRAP)
#define DEBUG_FAIL raise(SIGTRAP)
#else
#define DEBUG_FAIL raise(SIGABRT)
#endif

#define Assert(Condition) if(!(Condition)) { fprintf(stderr, "Error: %s (%s:%d)\n", #Condition, __FILE__, __LINE__); DEBUG_FAIL; };

#else

#define Assert(Condition)
#define DEBUG_FAIL

#endif

#define Unreachable Assert(!"Unreachable code")

// ---

#ifdef __cplusplus

namespace _impl {
    template<class _type> struct RmRef          { typedef _type type; };
    template<class _type> struct RmRef<_type&>  { typedef _type type; };
    template<class _type> struct RmRef<_type&&> { typedef _type type; };
}
template<class type>
using RmRef = typename _impl::RmRef<type>::type;
#define array_element_type(Array) RmRef<decltype(Array[0])>

#define SubscriptOperator(type) \
    inline \
    type& At(s64 Position) const { \
        Assert(Position >= 0); \
        Assert(Position < Length); \
        return Contents[Position]; \
    }
#else
// NOTE: This is here because SubscriptOperator is used in string, which is defined in basic_types.h
#define SubscriptOperator(type)
#endif

#include "basic_types.h"

#ifdef __cplusplus

template<class type>
struct array {
    s64 Length;
    type* Contents;

    SubscriptOperator(type);
};

template<class type>
struct dynamic_array {
    s64   Capacity;
    s64   Length;
    type* Contents;

    SubscriptOperator(type);
};

#undef SubscriptOperator

template<>
struct array<void> {
    s64 Length;
    void* Contents;
};

#define EmptyArray(type) (array<type> { 0, 0 })
#define Array(type, Array) (array<type> { ArrayLength(Array), (type*)(Array) })
#define ArrayWithLength(type, Length, Array) (array<type> { (Length), (type*)(Array) })
#define ArrayFromC(Array) (array<array_element_type(Array)> { ArrayLength(Array), (array_element_type(Array)*)(Array) })
// #define ArrayBuffer(Name, Size) char Name ## _ [Size]; string Name = String(Name ## _)

#endif

#define ArrayLength(Array) (sizeof(Array) / sizeof((Array)[0]))

#define ArrayAsArgs(Array) (int)(Array).Length, (Array).Contents
#define CArrayAsArgs(Array) (int)ArrayLength(Array), Array

#define EmptyString (string { 0, 0 })
// For string literals
#define StringLength(String) (ArrayLength(String) - 1)
#define String(Str) (string { StringLength(Str), (char*)(Str) })
// For char*
#define StringFromC(String) (string { CStringLength((char*)String), (char*)(String) })
#define StringWithLength(String, Length) (string { (Length), (char*)(String) })

#define StringBuffer(Name, Size) char Name ## _ [Size]; string Name = String(Name ## _)

#define StringAsArgs(String) (int)(String).Length, (String).Contents

// Struct Utilities
#define OffsetOf(struct_name, MemberName) ((int64_t)&(((struct_name*)0)->MemberName))
#define AtOffset(ValuePtr, Offset) (*(((int8_t*)ValuePtr) + Offset))

// ---

#endif
