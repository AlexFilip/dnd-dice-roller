/*
   File: common_operations.cpp
   Date: 17 July 2021
   Creator: Alexandru Filip
   Notice: (C) Copyright 2021 by Alexandru Filip. All rights reserved.
   */

internal int_size
CStringLength(char* CString) {
    int_size Result = 0;
    while(*CString++) {
        Result++;
    }
    return Result;
}

internal void
ClearBytes(void* Bytes, int_size Length) {
    for(int_size Index = 0; Index < Length; ++Index) {
        ((uint8_t*)Bytes)[Index] = 0;
    }
}

internal int_size
StringToIntUnchecked(string String) {
    int_size Result = 0;

    for(int_size Index = 0; Index < String.Length; ++Index) {
        Result = Result * 10 + (String.Contents[Index] - '0');
    }

    return Result;
}

internal void*
AllocateOnHeap(int_size Size) {
    // printf("Allocating %d on heap\n", Size);
    void* Result = malloc(Size);
    return Result;
}

#ifdef __cplusplus
template<class type> internal type*
AllocateOnHeapTyped(int_size Count = 1, int_size Extra = 0) {
    // printf("Allocating %d on heap\n", Size);
    type* Result = (type*)AllocateOnHeap(sizeof(type) * Count + Extra);
    return Result;
}
#endif

internal void
DeallocateHeap(void* Existing) {
    free(Existing);
}

internal void*
ReallocateOnHeap(void* Existing, int_size Size) {
    void* Result = NULL;
    if(Existing != NULL) {
        Result = realloc(Existing, Size);
    } else {
        Result = malloc(Size);
    }
    return Result;
}

#ifdef __cplusplus
template<class element> internal array<element> 
AllocateArray(int_size Length) {
    array<element> Result = {};

    Result.Length = Length;
    Result.Contents = AllocateOnHeapTyped<element>(Length);

    return Result;
}

template<class element> internal void
DeallocateArray(array<element>* Array) {
    if(Array->Contents != NULL) {
        DeallocateHeap(Array->Contents);
        Array->Contents = NULL;
        Array->Length = 0;
    }
}

template<class element> internal void
Deallocate(array<element> Array) {
    DeallocateArray(&Array);
}

template<class element> internal array<element> 
CopyOnHeap(array<element> Array) {
    array<element> Result = {};

    Result.Length = Array.Length;
    Result.Contents = AllocateOnHeapTyped<element>(Array.Length);

    for(int_size Index = 0; Index < Array.Length; ++Index) {
        Result.Contents[Index] = Array.Contents[Index];
    }

    return Result;
}

template<class type> internal array<type> 
ArrayOf() {
    array<type> Result = {};
    return Result;
}

template<class type1, class ...type> internal inline array<type1> 
ArrayOf(type1 Val1, type ... Vals) {
    type1 Args[] = { Val1, static_cast<type1>(Vals)... };
    array<type1> Result = AllocateArray<type1>(sizeof...(type) + 1);

    for(int_size Index = 0; Index < Result.Length; ++Index) {
        Result.Contents[Index] = Args[Index];
    }

    return Result;
}

template<class element> internal s64
ArrayConcat(array<element> Array1, array<element> Array2, array<element> Result) {
    s64 ResultIndex = 0;

    for(s64 Index1 = 0; Index1 < Array1.Length && ResultIndex < Result.Length; ++Index1, ++ResultIndex) {
        Result.At(ResultIndex) = Array1.At(Index1);
    }

    for(s64 Index2 = 0; Index2 < Array2.Length && ResultIndex < Result.Length; ++Index2, ++ResultIndex) {
        Result.At(ResultIndex) = Array2.At(Index2);
    }

    return ResultIndex;
}

// ---

template<class element> internal dynamic_array<element> 
ToDynamicArray(array<element> Array, s64 Capacity = 0) {
    dynamic_array<element> Result = {};

    Result.Length = Array.Length;
    Result.Contents = Array.Contents;
    Result.Capacity = Capacity < Array.Length ? Array.Length : Capacity;

    return Result;
}

template<class element> internal array<element> 
ToArray(dynamic_array<element> Array) {
    array<element> Result = {};

    Result.Length = Array.Length;
    Result.Contents = Array.Contents;

    return Result;
}

template<class element> internal dynamic_array<element> 
AllocateDynamicArray(int_size Capacity) {
    dynamic_array<element> Result = {};

    Result.Capacity = Capacity;
    Result.Contents = AllocateOnHeapTyped<element>(Capacity);

    return Result;
}

template<class element> internal void
DeallocateDynamicArray(dynamic_array<element>* Array) {
    if(Array->Contents != NULL) {
        DeallocateHeap(Array->Contents);
        *Array = {};
    }
}

template<class type> internal dynamic_array<type> 
DynamicArrayOf() {
    dynamic_array<type> Result = {};
    return Result;
}

template<class type1, class ...type> internal dynamic_array<type1> 
DynamicArrayOf(type1 Val1, type ... Vals) {
    type1 Args[] = { Val1, static_cast<type1>(Vals)... };
    dynamic_array<type1> Result = AllocateDynamicArray<type1>(ArrayLength(Args));
    Result.Length = Result.Capacity;

    for(int_size Index = 0; Index < Result.Capacity; ++Index) {
        Result.At(Index) = Args[Index];
    }

    return Result;
}

template<class element> internal s64
ArrayConcat(array<element> Array1, array<element> Array2, dynamic_array<element> Result) {
    s64 ResultIndex = 0;
    for(s64 Index1 = 0; Index1 < Array1.Length && ResultIndex < Result.Capacity; ++Index1, ++ResultIndex) {
        Result.At(ResultIndex) = Array1.At(Index1);
    }

    for(s64 Index2 = 0; Index2 < Array2.Length && ResultIndex < Result.Length; ++Index2, ++ResultIndex) {
        Result.At(ResultIndex) = Array2.At(Index2);
    }

    return ResultIndex;
}

template<class element> internal void
Reserve(dynamic_array<element>* Array, s64 DesiredSize) {
    if(Array->Capacity < DesiredSize) {
        s64 NewCapacity = Array->Capacity + (Array->Capacity >> 1);
        if(NewCapacity < 16) {
            NewCapacity = 16;
        }
        if(NewCapacity < DesiredSize) {
            NewCapacity = DesiredSize;
        }

        Assert(NewCapacity >= DesiredSize);

        // TODO: Custom allocation
        Array->Contents = (element*)ReallocateOnHeap(Array->Contents, sizeof(element) * NewCapacity);
        Array->Capacity = NewCapacity;
    }
}

template<class type> internal void
Append(dynamic_array<type>* Array, type Value) {
    Reserve(Array, Array->Length + 1);
    Array->At(Array->Length++) = Value;
}

template<class type> internal void
Append(dynamic_array<type>* Array, array<type> Values) {
    Reserve(Array, Array->Length + Values.Length);
    for(int_size Index = 0; Index < Values.Length; ++Index, Array->Length++) {
        Array->Contents[Array->Length] = Values.At(Index);
    }
}
#endif

// ---

internal string
AllocateString(int_size Length) {
    char* Buffer =  AllocateOnHeapTyped<char>(Length);
    string Result = StringWithLength(Buffer, Length);
    return Result;
}

internal void
DeallocateString(string* Str) {
    if(Str->Contents != NULL) {
        DeallocateHeap(Str->Contents);
        Str->Contents = NULL;
        Str->Length = 0;
    }
}

internal void
CopyInto(string String, char* Buffer) {
    for(int_size Index = 0; Index < String.Length; ++Index) {
        Buffer[Index] = String.At(Index);
    }
}

internal string
CopyOnHeap(string String) {
    char* Buffer = AllocateOnHeapTyped<char>(String.Length + 1);
    CopyInto(String, Buffer);
    Buffer[String.Length] = 0;
    string Result = StringWithLength(Buffer, String.Length);
    return Result;
}

internal string
StringFittingSize(int_size Size, string Existing) {
    string Result = Existing;
    if(Result.Length < Size) {
        DeallocateString(&Result);
        Result = AllocateString(Size);
    }
    return Result;
}

internal b32
StringsEqual(string String1, string String2) {
    int_size Result = 0;
    if(String1.Length == String2.Length) {
        Result = 1;
        for(int_size Index = 0; Index < String1.Length; ++Index) {
            if(String1.At(Index) != String2.At(Index)) {
                Result = 0;
                break;
            }
        }
    }
    return Result;
}

internal s64
StringConcat(string String1, string String2, string Result) {
    s64 ResultIndex = 0;

    for(s64 Index1 = 0; Index1 < String1.Length && ResultIndex < Result.Length; ++Index1, ++ResultIndex) {
        Result.At(ResultIndex) = String1.At(Index1);
    }

    for(s64 Index2 = 0; Index2 < String2.Length && ResultIndex < Result.Length; ++Index2, ++ResultIndex) {
        Result.At(ResultIndex) = String2.At(Index2);
    }

    return ResultIndex;
}

internal char*
ReadEntireFile(char const* Filename) {
    FILE* File = fopen(Filename, "rb");
    char* Result = {};

    if(File) {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result = AllocateOnHeapTyped<char>(FileSize + 1);

        fread(Result, FileSize, 1, File);
        Result[FileSize] = 0;

        fclose(File);
    }

    return Result;
}

#ifdef __cplusplus
template<class type> internal type
Identity(type Value) {
    return Value;
}

// TODO: Remake this so it can be used in C. The only thing you have to replace is KeyFn and element*
// NOTE: This assumes that all numbers are positive
template<class element, class key_fn = int_size(*)(element)> internal void
RadixSort(array<element> Array, array<element> Buffer, key_fn KeyFn = Identity<element>) {
#define NumRadixBits 4
#define NumSortKeys  (1 << NumRadixBits)
#define Mask         (NumSortKeys - 1)

    Assert(Buffer.Length >= Array.Length);

    for(s32 Shift = 0; Shift < sizeof(KeyFn(Array.At(0))) * 8; Shift += NumRadixBits) {
        s32 SortKeyCounts[NumSortKeys] = {};
        for(int_size Index = 0; Index < Array.Length; ++Index) {
            ++SortKeyCounts[(KeyFn(Array.At(Index)) >> Shift) & Mask];
        }

        for(int_size CountIndex = 1; CountIndex < NumSortKeys; ++CountIndex) {
            SortKeyCounts[CountIndex] += SortKeyCounts[CountIndex - 1];
        }

        for(int_size Index = Array.Length - 1; Index >= 0; --Index) {
            uint32_t SortKeyIndex = (KeyFn(Array.At(Index)) >> Shift) & Mask;
            uint32_t BufferIndex = --SortKeyCounts[SortKeyIndex];
            Buffer.At(BufferIndex) = Array.At(Index);
        }

        // NOTE: Exchange `Contents` pointers since we allow `BufferLength` to be >= `Array.Length`
        // and we want to make sure that `Array.Length` stays the same throughout.
        element* Temp = Array.Contents;
        Array.Contents = Buffer.Contents;
        Buffer.Contents = Temp;
    }

#undef NumSortKeys
#undef NumRadixBits
}

#endif


