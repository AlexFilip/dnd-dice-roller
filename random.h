/*
  File: Random.hpp
  Date: 01 April 2021
  Creator: Alexandru Filip
*/

#ifndef RANDOM_HPP
#define RANDOM_HPP

#include <stdint.h>
#include <math.h>

#ifndef ArrayLength
#define ArrayLength(Array) ( sizeof(Array) / sizeof((Array)[0]) )
#endif

#ifdef USE_STANDARD_C_RNG
#include <stdlib.h>
struct standard_c_random_state {
    // NOTHING
};

standard_c_random_state StandardCRNGSeed(unsigned s32 Seed);
s32 NextRandom(standard_c_random_state* Unused);
#endif

// TODO: find a way to seed these from just 1 number

// Linear Congruential Generator
struct pcg_random_state {
    u64 State;
    u64 Increment;
};

struct wichmann_hill_random_state1 {
    s32 S1, S2, S3;
};

struct wichmann_hill_random_state2 {
    s32 S1, S2, S3, S4;
};

// u32 NextRandom(bad_lcg_state* RandomState);

pcg_random_state PCGSeed(u64 InitialState, u64 InitialIncrement = 0);
u32 NextRandom(pcg_random_state* RandomState);

float NextRandom(wichmann_hill_random_state1* State);
float NextRandom(wichmann_hill_random_state2* State);

template<class element, class random_state>
void MakeRandomArray(int_size Length, element Array[], random_state* RandomState) {
    for(int_size Index = 0; Index < Length; ++Index) {
        Array[Index] = (element)NextRandom(RandomState);
    }
}

template<class element, class random_state>
void ShuffleArray(int_size Length, element Array[], random_state* RandomState) {
    for(int_size Index = 0; Index < Length-1; ++Index) {
        int_size RandomIndex = NextRandom(RandomState) % (Length - Index) + Index;

        element Temp = Array[Index];
        Array[Index] = Array[RandomIndex];
        Array[RandomIndex] = Temp;
    }
}

#endif

