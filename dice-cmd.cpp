/*
  File: dice-cmd.cpp
  Date: 09 June 2022
  Creator: Alexandru Filip
  Notice: (C) Copyright 2022 by Alexandru Filip. All rights reserved.
*/

struct dice_set {
    int Count;
    int NumSides;
};

enum token_type {
    TokenTypeNone,
    TokenTypeEndOfStream,
    TokenTypeError,
    TokenTypeDice,
    TokenTypeInt,
    TokenTypeString,
    TokenTypeIdentifier,
};

struct token {
    token_type Type;

    union {
        string ErrorMessage;
        dice_set Dice;
        string Identifier;
        string String;
        int Number;
    };
};

struct tokenizer {
    char* At;
    char* End;

    token LastReadToken;
    bool  LastReadIsValid;
};

internal token
GetToken(tokenizer* Tokenizer) {
    // NOTE: Error messages should be copied onto the heap and strings that are
    // part of the command should just refer to their places in the buffer.
    token Result = {};

    if(Tokenizer->LastReadIsValid) {
        Result = Tokenizer->LastReadToken;
        Tokenizer->LastReadIsValid = false;
    } else {
        // Skip whitespace. last character assumed to be zero
        while(IsWhitespace(Tokenizer->At[0])) {
            ++Tokenizer->At;
        }

        // string ErrorMessage = {};
        if(Tokenizer->At >= Tokenizer->End) {
            Result.Type = TokenTypeEndOfStream;
        } else {
            char Char = Tokenizer->At[0];
            int TokenEndIndex = 0;

            if(Char == '"' || Char == '\'') {
                do {
                    ++TokenEndIndex;
                } while(Tokenizer->At + TokenEndIndex < Tokenizer->End && Tokenizer->At[TokenEndIndex] != Char);

                if(Tokenizer->At[TokenEndIndex] == Char) {
                    Result.String = StringWithLength(Tokenizer->At + 1, TokenEndIndex - 1);
                    Result.Type = TokenTypeString;
                    Tokenizer->At += TokenEndIndex + 1; // +1 to count for second quote
                } else {
                    Result.ErrorMessage = String("Could not find terminating quote in string");
                    Result.Type = TokenTypeError;
                }
            } else if(Char == '+') {
                Tokenizer->At += 1;
            } else if(Char == '-') {
                Tokenizer->At += 1;
            } else if(IsNumber(Char) || IsLetter(Char)) {
                bool StartsWithNumber = IsNumber(Char);
                int StartNumber = 0;
                while(IsNumber(Tokenizer->At[TokenEndIndex])) {
                    StartNumber *= 10;
                    StartNumber += Tokenizer->At[TokenEndIndex] - '0';
                    ++TokenEndIndex;
                }

                const int FirstIndexAfterNumber = TokenEndIndex;
                bool HasLetters = IsLetter(Tokenizer->At[TokenEndIndex]);
                for(;;) {
                    if(IsNumber(Char)) {
                        // Nothing
                    } else if(IsLetter(Char)) {
                        HasLetters = true;
                    } else {
                        break;
                    }

                    ++TokenEndIndex;
                    Char = Tokenizer->At[TokenEndIndex];
                }

                // TODO: Identifier parsed, check if it is a number or a die format
                if(!HasLetters) {
                    // Is Number, might be float
                    if(Char == '.') {
                        Result.ErrorMessage = String("Floating point numbers are not currently handled");
                        Result.Type = TokenTypeError;
                    } else {
                        // Int
                        Result.Number = StartNumber;
                        Result.Type = TokenTypeInt;
                    }
                } else {
                    string WordStr = StringWithLength(Tokenizer->At, TokenEndIndex);
                    if((Tokenizer->At[FirstIndexAfterNumber] | 0x20) == 'd' /* Case-insensitive comparison */) {
                        int NumberEndIndex = FirstIndexAfterNumber + 1;
                        int StartDiceIndex = NumberEndIndex;
                        while(IsNumber(Tokenizer->At[NumberEndIndex])) {
                            ++NumberEndIndex;
                        }

                        if(NumberEndIndex == TokenEndIndex) {
                            // Only number after 'd'
                            string NumSidesString = StringWithLength(Tokenizer->At + StartDiceIndex, TokenEndIndex - StartDiceIndex);
                            int NumSides = StringToIntUnchecked(NumSidesString);

                            if(NumSides > 0) {
                                if(StartsWithNumber) {
                                    if(StartNumber <= 0) {
                                        Result.ErrorMessage = String("Num dice must be greater than zero");
                                        Result.Type = TokenTypeError;
                                    } else {
                                        Result.Dice.Count = StartNumber;
                                    }
                                } else {
                                    Result.Dice.Count = 1;
                                }

                                if(Result.Type == TokenTypeNone) {
                                    Result.Dice.NumSides = NumSides;
                                    Result.Type = TokenTypeDice;
                                }
                            } else {
                                Result.ErrorMessage = String("Num sides must be greater than zero");
                                Result.Type = TokenTypeError;
                            }
                        }
                    }
                }

                if(Result.Type == TokenTypeNone) {
                    // string ErrStart = String("Unrecognized token: ");
                    // Result.ErrorMessage = AllocateString(ErrStart.Length + TokenEndIndex);
                    // StringConcat(ErrStart, StringWithLength(Tokenizer->At, TokenEndIndex), Result.ErrorMessage);
                    // Result.Type = TokenTypeError;

                    Result.Identifier = StringWithLength(Tokenizer->At, TokenEndIndex);
                    Result.Type = TokenTypeIdentifier;
                }
                Tokenizer->At += TokenEndIndex;

            } else {
                static char ErrorMessageBuffer[] = "Unknown character '\0'";
                Result.ErrorMessage = String(ErrorMessageBuffer);
                Result.ErrorMessage.Contents[Result.ErrorMessage.Length - 2] = Char;
                Result.Type = TokenTypeError;
                Tokenizer->At += 1;
            }
        }

        Tokenizer->LastReadToken = Result;
    }

    return Result;
}

internal token
PeekNextToken(tokenizer* Tokenizer) {
    token Result = {};
    if(Tokenizer->LastReadIsValid) {
        Result = Tokenizer->LastReadToken;
    } else {
        Result = GetToken(Tokenizer);
        Tokenizer->LastReadIsValid = true;
    }
    return Result;
}

