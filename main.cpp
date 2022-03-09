/*
  File: main.cpp
  Date: 18 February 2022
  Creator: Alexandru Filip
  Notice: (C) Copyright 2022 by Alexandru Filip. All rights reserved.
*/

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include <stdarg.h>

#include "common_defs.h"
#include "basic_types.h"
#include "common_operations.cpp"

/*
 * TODO:
 *  - Check format for die rolls, error message if unrecognized error
 *  - adding and removing items
 *  - Allow GetToken to treat individual words as strings even if it is not a token.
 *  - If the user types something and moves up or down in the line buffer, save the working line into the buffer and copy back when they go past the end of the line buffer
 */

#define UpArrow 259
#define DownArrow 258
#define LeftArrow 260
#define RightArrow 261

#define Backspace 263
#define Delete 330

struct position {
    int X;
    int Y;
};

internal position
CurrentPosition() {
    position result = {};

    getyx(stdscr, result.Y, result.X);

    return result;
}

internal position
WindowSize() {
    position result = {};

    getmaxyx(stdscr, result.Y, result.X);

    return result;
}

internal void
MoveCursor(position Position) {
    move(Position.Y, Position.X);
}

internal void
DebugPrint(char const* format, ...) {
    position StartPos = CurrentPosition();
    position win_size = WindowSize();

    move(win_size.Y - 1, 0);

    char Buffer[4096];

    va_list args;
    va_start(args, format);

    vsnprintf(Buffer, ArrayLength(Buffer), format, args);

    va_end(args);

    wclrtoeol(stdscr);
    wprintw(stdscr, Buffer);

    MoveCursor(StartPos);
}

enum token_type {
    TokenTypeEndOfStream,
    TokenTypeError,
    TokenTypeQuit,
    TokenTypeAddItem,
    TokenTypeRemoveItem,
    TokenTypeDice,
    TokenTypeNumber,
    TokenTypeString,
};

struct token {
    token_type Type;

    union {
        string ErrorMessage;
        struct {
            int Count;
            int NumSides;
        } Dice;
        int Number;
    };
};

struct tokenizer {
    char* At;
    char* End;
};

token GetToken(tokenizer* Tokenizer) {
    token Result = {};

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

        if(Char == '"') {
            do {
                ++TokenEndIndex;
                Char = Tokenizer->At[TokenEndIndex];
            } while(Tokenizer->At + TokenEndIndex < Tokenizer->End && Char != '"');
            ++TokenEndIndex;

            // INCOMPLETE
            // Token is string
            Result.Type = TokenTypeString;
        } else {
            while(IsNumber(Char) || IsLetter(Char)) {
                ++TokenEndIndex;
                Char = Tokenizer->At[TokenEndIndex];
            }
        }

        if(TokenEndIndex > 0) {
            string WordStr = StringWithLength(Tokenizer->At, TokenEndIndex);
            if(StringsEqual(WordStr, String("add"))) {
                Result.Type = TokenTypeAddItem;
            } else if(StringsEqual(WordStr, String("remove"))) {
                Result.Type = TokenTypeRemoveItem;
            } else if(StringsEqual(WordStr, String("quit")) || StringsEqual(WordStr, String("exit"))) {
                Result.Type = TokenTypeQuit;
            } else {
                int NumDice = 1;
                int DiceIndex = 0;
                while(IsNumber(Tokenizer->At[DiceIndex])) {
                    ++DiceIndex;
                }

                if(DiceIndex > 0) {
                    string NumDiceString = StringWithLength(Tokenizer->At, DiceIndex);
                    NumDice = StringToIntUnchecked(NumDiceString);
                }

                if(NumDice == 0) {
                    Result.ErrorMessage = CopyOnHeap(String("Num dice must be greater than zero"));
                    Result.Type = TokenTypeError;
                } else if(DiceIndex == TokenEndIndex) {
                    Result.Number = NumDice;
                    Result.Type = TokenTypeNumber;
                } else if((Tokenizer->At[DiceIndex] | 0x20) == 'd' /* Case-insensitive comparison */) {
                    ++DiceIndex;
                    int StartDiceIndex = DiceIndex;
                    while(IsNumber(Tokenizer->At[DiceIndex])) {
                        ++DiceIndex;
                    }

                    if(DiceIndex == StartDiceIndex) {
                        Result.ErrorMessage = CopyOnHeap(String("You must provide the number of sides a dice has."));
                        Result.Type = TokenTypeError;
                    } else if(DiceIndex < TokenEndIndex) {
                        Result.ErrorMessage = CopyOnHeap(String("Trailing characters after number of sides."));
                        Result.Type = TokenTypeError;
                    } else {
                        string NumSidesString = StringWithLength(Tokenizer->At + StartDiceIndex, TokenEndIndex - StartDiceIndex);
                        int NumSides = StringToIntUnchecked(NumSidesString);
                        
                        if(NumSides == 0) {
                            Result.ErrorMessage = CopyOnHeap(String("Num sides must be greater than zero"));
                            Result.Type = TokenTypeError;
                        } else {
                            Result.Dice.Count    = NumDice;
                            Result.Dice.NumSides = NumSides;
                            Result.Type = TokenTypeDice;
                        }
                    }
                } else {
                    string ErrStart = String("Unrecognized token: ");
                    AllocateString(ErrStart.Length + TokenEndIndex);
                    StringConcat(ErrStart, StringWithLength(Tokenizer->At, TokenEndIndex), Result.ErrorMessage);
                    Result.Type = TokenTypeError;
                }
            }
            Tokenizer->At += TokenEndIndex;
        } else {
            Result.ErrorMessage = CopyOnHeap(String("Unknown character '-'"));
            Result.ErrorMessage.Contents[Result.ErrorMessage.Length - 2] = Char;
            Result.Type = TokenTypeError;
            Tokenizer->At += 1;
        }

    }

    return Result;
}

char const prompt[] = "> ";

int main() {
    char Buffer[100];
    srand(time(NULL));

    dynamic_array<string> LineBuffer = {};
    int LineBufferPosition = 0;
    bool IsRunning = false;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    IsRunning = true;
    while(IsRunning) {
        move(0, 0);
        addstr(prompt);
        refresh();

        int BufferIndex = 0;
        int BufferLength = 0;
        memset(Buffer, 0, ArrayLength(Buffer));
        LineBufferPosition = 0;

        bool TypedFirstKey = false;
        for(;;) {
            int Char = getch();
            // DebugPrint("Char is %d", Char);

            if(!TypedFirstKey) {
                wclrtoeol(stdscr);
                TypedFirstKey = true;
            }

            if(isprint(Char)) {
                if(BufferLength < StringLength(Buffer)) {
                    char NextLetter = (char) Char;
                    int StartBufferIndex = BufferIndex;
                    int CurrentIndex = StartBufferIndex;
                    while(CurrentIndex < StringLength(Buffer) && NextLetter != 0) {
                        char temp = Buffer[CurrentIndex];
                        Buffer[CurrentIndex] = NextLetter;
                        NextLetter = temp;
                        ++CurrentIndex;
                    }
                    Buffer[CurrentIndex] = '\0';
                    ++BufferIndex;
                    ++BufferLength;

                    position StartPos = CurrentPosition();
                    CurrentIndex = StartBufferIndex;
                    while(CurrentIndex < StringLength(Buffer) && Buffer[CurrentIndex] != 0) {
                        addch(Buffer[CurrentIndex]);
                        CurrentIndex++;
                    }

                    ++StartPos.X;
                    MoveCursor(StartPos);
                }
            } else {
                if(Char == '\n') {
                    if(BufferLength != 0) {
                        wclrtobot(stdscr);
                        position CurrentPos = CurrentPosition();
                        move(CurrentPos.Y + 1, 0);

                        // DebugPrint("Command is %s\n", Buffer);

                        string HeapString = CopyOnHeap(StringFromC(Buffer));
                        Append(&LineBuffer, HeapString);

                        break;
                    }
                } else if(Char == KEY_UP) {
                    if(LineBufferPosition >= 0 && LineBufferPosition < LineBuffer.Length) {
                        if(LineBufferPosition == 0) {
                            // line currently being typed
                        }

                        ++LineBufferPosition;
                        string PrevString = LineBuffer.At(LineBuffer.Length - LineBufferPosition);
                        CopyInto(PrevString, Buffer);

                        Buffer[PrevString.Length] = '\0';
                        BufferIndex = PrevString.Length;
                        BufferLength = PrevString.Length;

                        // redraw line
                        move(0, StringLength(prompt));
                        wclrtoeol(stdscr);
                        addstr(Buffer);
                    }
                } else if(Char == KEY_DOWN) {
                    if(LineBufferPosition >= 0 && LineBufferPosition <= LineBuffer.Length) {
                        string NextString = {};

                        if(LineBufferPosition == 0) {
                            // TODO: Copy working line into buffer
                        } else {
                            --LineBufferPosition;
                            NextString = LineBuffer.At(LineBuffer.Length - LineBufferPosition);
                        }

                        CopyInto(NextString, Buffer);

                        Buffer[NextString.Length] = '\0';
                        BufferIndex = NextString.Length;
                        BufferLength = NextString.Length;

                        // redraw line
                        move(0, StringLength(prompt));
                        wclrtoeol(stdscr);
                        addstr(Buffer);

                    }
                } else if(Char == KEY_LEFT) {
                    if(BufferIndex > 0) {
                        --BufferIndex;
                        position Cursor = CurrentPosition();
                        --Cursor.X;
                        MoveCursor(Cursor);
                    }
                } else if(Char == KEY_RIGHT) {
                    if(Buffer[BufferIndex] != 0 && BufferIndex < BufferLength) {
                        ++BufferIndex;
                        position Position = CurrentPosition();
                        ++Position.X;
                        MoveCursor(Position);
                    }
                } else if(Char == 127 || Char == KEY_BACKSPACE || Char == KEY_DC) {
                    if(BufferIndex > 0) {
                        --BufferIndex;
                        for(int Index = BufferIndex; Index < StringLength(Buffer) && Buffer[Index] != 0; ++Index) {
                            Buffer[Index] = Buffer[Index + 1];
                        }
                        position Cursor = CurrentPosition();
                        --Cursor.X;
                        MoveCursor(Cursor);
                        // this function takes care of all the on screen deleting on a line
                        wdelch(stdscr);
                    }
                }
                // TODO: Check for control and special characters
            }

            refresh();
        }

        char* Text = Buffer;

        tokenizer Tokenizer = {};
        Tokenizer.At = Buffer;
        Tokenizer.End = Buffer + BufferLength;
        *(Tokenizer.End) = '\0';

        bool IsReading = true;
        while(IsReading) {
            token CurrentToken = GetToken(&Tokenizer);

            if(CurrentToken.Type == TokenTypeEndOfStream) {
                IsReading = false;
            } else if(CurrentToken.Type == TokenTypeAddItem) {
                token ItemToAddToken = GetToken(&Tokenizer);
                // TODO: Add item
                printw("Item adding is incomplete");
            } else if(CurrentToken.Type == TokenTypeRemoveItem) {
                token ItemToRemoveToken = GetToken(&Tokenizer);
                // TODO: Remove item
                printw("Item removing is incomplete");
            } else if(CurrentToken.Type == TokenTypeQuit) {
                IsRunning = false;
                IsReading = false;
            } else if(CurrentToken.Type == TokenTypeDice) {
                // printw("Rolling %d dice with %d sides\n", CurrentToken.Dice.Count, CurrentToken.Dice.NumSides);
                // TODO: I might want to factor this out into its own function in case I want to add dice as items. Then I can use it later.
                int Total = 0;
                int Max = 0;
                int Min = 0x7FFFFFFF;
                
                for(int Index = 0; Index < CurrentToken.Dice.Count; ++Index) {
                    int Num = rand() % CurrentToken.Dice.NumSides + 1;
                    Total += Num;

                    if(Num > Max) {
                        Max = Num;
                    }

                    if(Num < Min) {
                        Min = Num;
                    }
                }

                // This weird way of printing the dice and checking that the count is 1 two times is here
                // because using "%dd%d" as a format string leads to weird output.
                if(CurrentToken.Dice.Count != 1) {
                    printw("%d", CurrentToken.Dice.Count);
                }
                printw("d%d:\n", CurrentToken.Dice.NumSides);

                if(CurrentToken.Dice.Count == 1) {
                    printw("  %d\n\n", Total);
                } else {
                    printw("  Total: %d\n"
                           "  Max: %d\n"
                           "  Min: %d\n\n",
                           Total, Max, Min);
                }

            } else if(CurrentToken.Type == TokenTypeNumber) {
                printw("Error: Cannot start command with number\n");
            } else if(CurrentToken.Type == TokenTypeString) {
                printw("Error: Cannot start command with string\n");
            } else if(CurrentToken.Type == TokenTypeError) {
                printw("Error: %.*s\n", StringAsArgs(CurrentToken.ErrorMessage));
                DeallocateString(&CurrentToken.ErrorMessage);
                IsReading = false;
            }
            refresh();
        }
    }

    endwin();

    return 0;
}
