/*
  File: Random.cpp
  Date: 31 March 2021
  Creator: Alexandru Filip
*/

#include "random.h"

#ifdef USE_STANDARD_C_RNG
standard_c_random_state StandardCRNGSeed(u32 Seed) {
    srand(Seed);
    standard_c_random_state Result = {};
    return Result;
}

s32 NextRandom(standard_c_random_state*) {
    s32 Result = rand();
    return Result;
}
#endif

// TODO: These rotate functions seem like they're more fit for a numerics library
#define RotateLeftImpl(type) \
    type RotateLeft(type Number, type Rotation) { \
        return (Number << Rotation) | (Number >> ((-Rotation) & (sizeof(type)*8 - 1))); \
    }
RotateLeftImpl(u32)
RotateLeftImpl(u64)
#undef RotateLeftImpl

#define RotateRightImpl(type) \
    type RotateRight(type Number, type Rotation) { \
        return (Number >> Rotation) | (Number << ((-Rotation) & (sizeof(type)*8 - 1))); \
    }
RotateRightImpl(u32)
RotateRightImpl(u64)
#undef RotateRightImpl

u32 NextRandom(pcg_random_state* RandomState) {
    u64 OldState = RandomState->State;
    RandomState->State = OldState * 6364136223846793005ULL + (RandomState->Increment | 1);

    u32 Result = ((OldState >> 18u) ^ OldState) >> 27u;
    u32 Rotation = OldState >> 59u;
    Result = RotateRight(Result, Rotation);
    return Result;
}

pcg_random_state PCGSeed(u64 InitialState, u64 InitialIncrement) {
    pcg_random_state Result = {};
    Result.State = 0u;
    Result.Increment = (InitialIncrement << 1u) | 1u;

    NextRandom(&Result);
    Result.State += InitialState;
    NextRandom(&Result);

    return Result;
}

// Given the state of an RNG returns a number between 0 and 1 (inclusive?)
float NextRandom(wichmann_hill_random_state1* State) {
    s32 S1 = (171 * State->S1) % 30269,
        S2 = (172 * State->S2) % 30307,
        S3 = (170 * State->S3) % 30323;
    *State = { S1, S2, S3 };
    return fmod((float)S1/30269.0 + (float)S2/30307.0 + (float)S3/30323.0, 1.0);
}

// Given the state of an RNG returns a number between 0 and 1 (inclusive?)
float NextRandom(wichmann_hill_random_state2* State) {
    s32 S1 = (11600 * State->S1) % 2147483579,
        S2 = (47003 * State->S2) % 2147483543,
        S3 = (23000 * State->S3) % 2147483423,
        S4 = (33000 * State->S4) % 2147483123;

    *State = { S1, S2, S3, S4 };
    float Result =  fmod((float)S1/ 2147483579.0 +
                         (float)S2/ 2147483543.0 +
                         (float)S3/ 2147483423.0 +
                         (float)S4/ 2147483123.0,
                         1.0);
    return Result - floor(Result);
}
