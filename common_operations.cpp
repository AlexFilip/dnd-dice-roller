/*
   File: common_operations.cpp
   Date: 17 July 2021
   Creator: Alexandru Filip
   Notice: (C) Copyright 2021 by Alexandru Filip. All rights reserved.
   */

internal b32
IsNewLine(char Char) {
    b32 Result = ((Char == '\n') || (Char == '\r'));
    return Result;
}

internal b32
IsWhitespace(char Char) {
    b32 Result = ((Char == ' ') ||
                  (Char == '\t') ||
                  (Char == '\f') ||
                  IsNewLine(Char));
    return Result;
}

internal b32
IsLetter(char Char) {
    b32 Result = IsUpper(Char) || IsLower(Char);
    return Result;
}

internal b32
IsNumber(char Char) {
    b32 Result = IsDigit(Char);
    return Result;
}

internal int
CStringLength(char* CString) {
    int Result = 0;
    while(*CString++) {
        Result++;
    }
    return Result;
}

internal int
StringToIntUnchecked(string String) {
    int Result = 0;

    for(int Index = 0; Index < String.Length; ++Index) {
        Result = Result * 10 + (String.Contents[Index] - '0');
    }

    return Result;
}

internal void*
AllocateOnHeap(int Size) {
    // printf("Allocating %d on heap\n", Size);
    void* Result = malloc(Size);
    return Result;
}

#ifdef __cplusplus
template<class type> internal type*
AllocateOnHeapTyped(int Count = 1, int Extra = 0) {
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
ReallocateOnHeap(void* Existing, int Size) {
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
AllocateArray(int Length) {
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

    for(int Index = 0; Index < Array.Length; ++Index) {
        Result.Contents[Index] = Array.Contents[Index];
    }

    return Result;
}

template<class type> internal array<type> 
ArrayOf() {
    array<type> Result = {};
    return Result;
}

template<class type1, class ...type> internal array<type1> 
ArrayOf(type1 Val1, type ... Vals) {
    type1 Args[] = { Val1, static_cast<type1>(Vals)... };
    array<type1> Result = AllocateArray<type1>(sizeof...(type) + 1);

    for(int Index = 0; Index < Result.Length; ++Index) {
        Result.At(Index) = Args[Index];
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
AllocateDynamicArray(int Capacity) {
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

    for(int Index = 0; Index < Result.Capacity; ++Index) {
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
    for(int Index = 0; Index < Values.Length; ++Index, Array->Length++) {
        Array->Contents[Array->Length] = Values.At(Index);
    }
}
#endif

// ---

internal string
AllocateString(int Length) {
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
    for(int Index = 0; Index < String.Length; ++Index) {
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
StringFittingSize(int Size, string Existing) {
    string Result = Existing;
    if(Result.Length < Size) {
        DeallocateString(&Result);
        Result = AllocateString(Size);
    }
    return Result;
}

internal b32
StringsEqual(string String1, string String2) {
    int Result = 0;
    if(String1.Length == String2.Length) {
        Result = 1;
        for(int Index = 0; Index < String1.Length; ++Index) {
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

#ifdef __cplusplus
template<class type>
struct auto_free_holder {
    type Value;
    ~auto_free_holder() {
        // printf("Freeing\n");
        Deallocate(Value);
    }

    operator type() {
        return Value;
    }
};

// Usage: AutoFree(something) or directly use AutoFree_(something).Value if that causes problems
// The '.Value' part is a little ugly but this works because the auto_free_holder's
// destructor is only called after the line it's on finishes.
template<class type> internal auto_free_holder<type>
AutoFree_(type Value) {
    auto_free_holder<type> Result;

    Result.Value = Value;

    return Result;
}

#define AutoFree(Val) AutoFree_(Val).Value

template<class type> array<type>
Flatten(array<array<type>> ArrayOfArrays) {
    array<type> Result = {};

    int TotalLength = 0;
    for(int Index = 0; Index < ArrayOfArrays.Length; ++Index) {
        TotalLength += ArrayOfArrays.Contents[Index].Length;
    }

    Result = AllocateArray<type>(TotalLength);

    int ResultIndex = 0;
    for(int Index = 0; Index < ArrayOfArrays.Length; ++Index) {
        array<type> InnerArray = ArrayOfArrays.Contents[Index];
        for(int InnerIndex = 0; InnerIndex < InnerArray.Length; ++InnerIndex) {
            Result.Contents[ResultIndex] = InnerArray.Contents[InnerIndex];
            ResultIndex += 1;
        }
    }

    return Result;
}

template<class input_element, class transform_function> auto
Map(array<input_element> Array, transform_function Fn) -> array<decltype(Fn(Array.Contents[0]))> {
    typedef decltype(Fn(Array.Contents[0])) output_element;
    array<output_element> Result = {};

    Result = AllocateArray<output_element>(Array.Length);

    for(int Index = 0; Index < Array.Length; ++Index) {
        Result.Contents[Index] = Fn(Array.Contents[Index]);
    }

    return Result;
}

template<class input_element, class transform_function> auto
Map(array<input_element> Array, transform_function Fn) -> array<decltype(Fn(Array.Contents[0], 0))> {
    typedef decltype(Fn(Array.Contents[0], 0)) output_element;
    array<output_element> Result = {};

    Result = AllocateArray<output_element>(Array.Length);

    for(int Index = 0; Index < Array.Length; ++Index) {
        Result.Contents[Index] = Fn(Array.Contents[Index], Index);
    }

    return Result;
}
template<class input_element, class transform_function> auto
FlatMap(array<input_element> Array, transform_function Fn) -> array<RmRef<decltype(Fn(Array.Contents[0]).Contents[0])>> {
    typedef RmRef<decltype(Fn(Array.Contents[0]).Contents[0])> output_element;

    dynamic_array<output_element> Result = {};
    for(int Index = 0; Index < Array.Length; ++Index) {
        array<output_element> IntermediateOutput = Fn(Array.Contents[Index]);
        Append(&Result, IntermediateOutput);
    }

    return ToArray(Result);
}

template<class input_element, class transform_function> auto
FlatMap(array<input_element> Array, transform_function Fn) -> array<RmRef<decltype(Fn(Array.Contents[0], 0).Contents[0])>> {
    typedef RmRef<decltype(Fn(Array.Contents[0], 0).Contents[0])> output_element;

    dynamic_array<output_element> Result = {};
    for(int Index = 0; Index < Array.Length; ++Index) {
        array<output_element> IntermediateOutput = Fn(Array.Contents[Index], Index);
        Append(&Result, IntermediateOutput);
    }

    return ToArray(Result);
}

// template<class ...>
// Reduce(Initial value, Reducer) {
//    // TODO
// }

#endif

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
template<class element, class key_fn = int(*)(element)> internal void
RadixSort(array<element> Array, array<element> Buffer, key_fn KeyFn = Identity<element>) {
#define NumRadixBits 4
#define NumSortKeys  (1 << NumRadixBits)
#define Mask         (NumSortKeys - 1)

    Assert(Buffer.Length >= Array.Length);


    for(int Shift = 0; Shift < sizeof(KeyFn(Array.At(0))) * 8; Shift += NumRadixBits) {
        int SortKeyCounts[NumSortKeys] = {};
        for(int Index = 0; Index < Array.Length; ++Index) {
            ++SortKeyCounts[(KeyFn(Array.At(Index)) >> Shift) & Mask];
        }

        for(int CountIndex = 1; CountIndex < NumSortKeys; ++CountIndex) {
            SortKeyCounts[CountIndex] += SortKeyCounts[CountIndex - 1];
        }

        for(int Index = Array.Length - 1; Index >= 0; --Index) {
            uint32_t SortKeyIndex = (KeyFn(Array.At(Index)) >> Shift) & Mask;
            uint32_t BufferIndex = --SortKeyCounts[SortKeyIndex];
            Buffer.At(BufferIndex) = Array.At(Index);
        }

        // NOTE: Exchange `Contents` pointers since we allow `BufferLength` to be >= `Array.Length`
        // and we want to make sure that `Array.Length` stays the same throughout.
        element* Temp = Array.Contents;
        Array.Contents = Buffer.Contents;
        Buffer.Contents = Temp;

        // for(int Index = 0; Index < NumSortKeys; ++Index) {
        //     SortKeyCounts[Index] = 0;
        // }
    }

#undef NumSortKeys
#undef NumRadixBits
}

#endif


