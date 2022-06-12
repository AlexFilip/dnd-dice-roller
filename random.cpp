/*
  File: Random.cpp
  Date: 31 March 2021
  Creator: Alexandru Filip
*/

#include "random.h"

#ifdef USE_STANDARD_C_RNG
standard_c_random_state StandardCRNGSeed(unsigned int Seed) {
    srand(Seed);
    standard_c_random_state Result = {};
    return Result;
}

int NextRandom(standard_c_random_state*) {
    int Result = rand();
    return Result;
}
#endif

// TODO: These rotate functions seem like they're more fit for a numerics library
#define RotateLeftImpl(type) \
    type RotateLeft(type Number, type Rotation) { \
        return (Number << Rotation) | (Number >> ((-Rotation) & (sizeof(type)*8 - 1))); \
    }
RotateLeftImpl(uint32_t)
RotateLeftImpl(uint64_t)
#undef RotateLeftImpl

#define RotateRightImpl(type) \
    type RotateRight(type Number, type Rotation) { \
        return (Number >> Rotation) | (Number << ((-Rotation) & (sizeof(type)*8 - 1))); \
    }
RotateRightImpl(uint32_t)
RotateRightImpl(uint64_t)
#undef RotateRightImpl

uint32_t NextRandom(pcg_random_state* RandomState) {
    uint64_t OldState = RandomState->State;
    RandomState->State = OldState * 6364136223846793005ULL + (RandomState->Increment | 1);

    uint32_t Result = ((OldState >> 18u) ^ OldState) >> 27u;
    uint32_t Rotation = OldState >> 59u;
    Result = RotateRight(Result, Rotation);
    return Result;
}

pcg_random_state PCGSeed(uint64_t InitialState, uint64_t InitialIncrement) {
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
    int32_t S1 = (171 * State->S1) % 30269,
            S2 = (172 * State->S2) % 30307,
            S3 = (170 * State->S3) % 30323;
    *State = { S1, S2, S3 };
    return fmod((float)S1/30269.0 + (float)S2/30307.0 + (float)S3/30323.0, 1.0);
}

// Given the state of an RNG returns a number between 0 and 1 (inclusive?)
float NextRandom(wichmann_hill_random_state2* State) {
    int32_t S1 = (11600 * State->S1) % 2147483579,
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
