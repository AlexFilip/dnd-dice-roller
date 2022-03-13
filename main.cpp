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
 *  - Allow GetToken to treat individual words as strings even if it is not a valid token.
 *  - If the user types something and moves up or down in the line buffer, save the working line into the buffer and copy back when they go past the end of the line buffer
 *  - FIX: After resizing, deleting characters, then pressing enter, it will output 2 lines. The expected output and "Error: Unknown character '"
 *
 *  - adv, dis, sum/total after dice for advantage, disadvantage, total
 *  - labels on dice. ex. 2d20:attack, which will use "attack" instead of 2d20 in the result labels
 *  - comparison operators: >, <, >=, <= which just say true/false or succeeded/failed (ex. 2d12 > 7 will compare the total of 2d12 to 7)
 *    Q: What should the precedence be on labels vs. comparison operators?
 *
 *  Eventually (just for fun)
 *  - Math expressions that include dice. Ex. "2 * (2d20 + 4) - 3d8 > 7 - 3d6"
 */

#define UpArrow    259
#define DownArrow  258
#define LeftArrow  260
#define RightArrow 261

#define Backspace 263
#define Delete    330

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

struct dice_collection {
    int Count;
    int NumSides;
};

enum token_type {
    TokenTypeEndOfStream,
    TokenTypeError,
    TokenTypeQuit,
    TokenTypeAddItem,
    TokenTypeRemoveItem,
    TokenTypeDice,
    TokenTypeNumber,
    TokenTypeString,
    TokenTypeIdentifier,
};

struct token {
    token_type Type;

    union {
        string ErrorMessage;
        dice_collection Dice;
        string Identifier;
        string String;
        int Number;
    };
};

struct tokenizer {
    char* At;
    char* End;
};

token GetToken(tokenizer* Tokenizer) {
    // NOTE: Error messages should be copied onto the heap and strings that are
    // part of the command should just refer to their places in the buffer.
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
        } else {
            while(IsNumber(Char) || IsLetter(Char)) {
                ++TokenEndIndex;
                Char = Tokenizer->At[TokenEndIndex];
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
                        Result.ErrorMessage = String("Num dice must be greater than zero");
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
                            Result.ErrorMessage = String("You must provide the number of sides a die has.");
                            Result.Type = TokenTypeError;
                        } else if(DiceIndex < TokenEndIndex) {
                            Result.ErrorMessage = String("Trailing characters after number of sides.");
                            Result.Type = TokenTypeError;
                        } else {
                            string NumSidesString = StringWithLength(Tokenizer->At + StartDiceIndex, TokenEndIndex - StartDiceIndex);
                            int NumSides = StringToIntUnchecked(NumSidesString);

                            if(NumSides == 0) {
                                Result.ErrorMessage = String("Num sides must be greater than zero");
                                Result.Type = TokenTypeError;
                            } else {
                                Result.Dice.Count    = NumDice;
                                Result.Dice.NumSides = NumSides;
                                Result.Type = TokenTypeDice;
                            }
                        }
                    } else {
                        // string ErrStart = String("Unrecognized token: ");
                        // Result.ErrorMessage = AllocateString(ErrStart.Length + TokenEndIndex);
                        // StringConcat(ErrStart, StringWithLength(Tokenizer->At, TokenEndIndex), Result.ErrorMessage);
                        // Result.Type = TokenTypeError;

                        Result.Identifier = StringWithLength(Tokenizer->At, TokenEndIndex);
                        Result.Type = TokenTypeIdentifier;
                    }
                }
                Tokenizer->At += TokenEndIndex;
            } else {
                static char ErrorMessageBuffer[] = "Unknown character '-'";
                Result.ErrorMessage = String(ErrorMessageBuffer);
                Result.ErrorMessage.Contents[Result.ErrorMessage.Length - 2] = Char;
                Result.Type = TokenTypeError;
                Tokenizer->At += 1;
            }
        }
    }

    return Result;
}

char const Prompt[] = "> ";

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
        addstr(Prompt);
        refresh();

        int BufferIndex  = 0;
        int BufferLength = 0;
        memset(Buffer, 0, ArrayLength(Buffer));
        LineBufferPosition = 0;

        bool TypedFirstKey = false;
        for(;;) {
            int Char = getch();

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
                        move(0, StringLength(Prompt));
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
                        move(0, StringLength(Prompt));
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

                        for(int Index = BufferIndex; Index < BufferLength; ++Index) {
                            Buffer[Index] = Buffer[Index + 1];
                        }

                        Buffer[BufferLength] = 0;
                        --BufferLength;

                        position Cursor = CurrentPosition();
                        --Cursor.X;
                        MoveCursor(Cursor);
                        // this function takes care of all the on screen deleting on a line
                        wdelch(stdscr);
                    }
                } else if(Char == KEY_RESIZE) {
                    move(0, 0);
                    wclrtobot(stdscr);
                    addstr(Prompt);
                    addstr(Buffer);
                    move(0, BufferIndex + StringLength(Prompt));

                    // TODO: Save output and draw it 
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
                string ItemToAddString = {};

                if(ItemToAddToken.Type == TokenTypeIdentifier) {
                    ItemToAddString = ItemToAddToken.Identifier;
                } else if(ItemToAddToken.Type == TokenTypeString) {
                    ItemToAddString = ItemToAddToken.String;
                } else if(ItemToAddToken.Type == TokenTypeDice) {
                    // ItemToAddString = CopyOnHeap(String());
                } else {
                    printw("Invalid token given for item name. Expected word or string.");
                    IsReading = false;
                }

                // TODO: Add item
                if(ItemToAddString.Length > 0) {
                    printw("Will add item '%.*s'\n", StringAsArgs(ItemToAddString));
                }
            } else if(CurrentToken.Type == TokenTypeRemoveItem) {
                token ItemToRemoveToken = GetToken(&Tokenizer);
                string ItemToRemoveString = {};

                if(ItemToRemoveToken.Type == TokenTypeIdentifier) {
                    ItemToRemoveString = ItemToRemoveToken.Identifier;
                } else if(ItemToRemoveToken.Type == TokenTypeString) {
                    ItemToRemoveString = ItemToRemoveToken.String;
                } else if(ItemToRemoveToken.Type == TokenTypeDice) {
                } else {
                    printw("Invalid token given for item name. Expected word or string.");
                    IsReading = false;
                }

                // TODO: Remove item
                if(ItemToRemoveString.Length > 0) {
                    printw("Will remove item '%.*s'\n", StringAsArgs(ItemToRemoveString));
                }
            } else if(CurrentToken.Type == TokenTypeQuit) {
                IsRunning = false;
                IsReading = false;
            } else if(CurrentToken.Type == TokenTypeDice) {
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

            } else if(CurrentToken.Type == TokenTypeIdentifier) {
                printw("Error: '%.*s' is not a valid command\n", StringAsArgs(CurrentToken.Identifier));
            } else if(CurrentToken.Type == TokenTypeNumber) {
                printw("Error: Cannot start command with number\n");
            } else if(CurrentToken.Type == TokenTypeString) {
                printw("Error: Cannot start command with string\n");
            } else if(CurrentToken.Type == TokenTypeError) {
                printw("Error: %.*s\n", StringAsArgs(CurrentToken.ErrorMessage));
                IsReading = false;
            }
        }
        refresh();
    }

    endwin();

    return 0;
}
