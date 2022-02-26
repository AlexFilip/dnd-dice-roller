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

char const prompt[] = "> ";

int main() {

    char Buffer[100];
    srand(time(NULL));

    dynamic_array<string> LineBuffer = {};
    int LineBufferPosition = 0;

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    int BufferIndex = 0;

    for(;;) {
        move(0, 0);
        addstr(prompt);
        refresh();

        BufferIndex = 0;
        memset(Buffer, 0, ArrayLength(Buffer));
        LineBufferPosition = 0;

        bool TypedFirstLetter = false;
        for(;;) {
            int Char = getch();
            // DebugPrint("Char is %d", Char);

            if(!TypedFirstLetter) {
                wclrtoeol(stdscr);
                TypedFirstLetter = true;
            }

            if(isprint(Char)) {
                if(BufferIndex < StringLength(Buffer)) {
                    char NextLetter = (char) Char;
                    int StartBufferIndex = BufferIndex;
                    int CurrentIndex = StartBufferIndex;
                    while(CurrentIndex < StringLength(Buffer) && NextLetter != 0) {
                        char temp = Buffer[CurrentIndex];
                        Buffer[CurrentIndex] = NextLetter;
                        NextLetter = temp;
                        ++CurrentIndex;
                    }
                    ++BufferIndex;

                    position StartPos = CurrentPosition();
                    CurrentIndex = StartBufferIndex;
                    while(CurrentIndex < StringLength(Buffer) && Buffer[CurrentIndex] != 0) {
                        addch(Buffer[CurrentIndex]);
                        CurrentIndex++;
                    }

                    ++StartPos.X;
                    MoveCursor(StartPos);
                    refresh();
                }
            } else {
                if(Char == '\n') {
                    wclrtobot(stdscr);
                    position CurrentPos = CurrentPosition();
                    move(CurrentPos.Y + 1, 0);
                    BufferIndex = 0;

                    // DebugPrint("Command is %s\n", Buffer);

                    string HeapString = CopyOnHeap(StringFromC(Buffer));
                    Append(&LineBuffer, HeapString);

                    break;
                } else if(Char == KEY_UP) {
                    if(LineBufferPosition >= 0 && LineBufferPosition < LineBuffer.Length) {
                        ++LineBufferPosition;
                        string PrevString = LineBuffer.At(LineBuffer.Length - LineBufferPosition);
                        // DebugPrint("Key up. buffer_pos = %d, PrevString length = %d", BufferIndex, PrevString.Length);
                        CopyInto(PrevString, Buffer);

                        // redraw line
                        move(0, StringLength(prompt));
                        wclrtoeol(stdscr);
                        addstr(Buffer);

                        refresh();

                        Buffer[PrevString.Length] = '\0';
                        BufferIndex = PrevString.Length;
                    }
                } else if(Char == KEY_DOWN) {
                    if(LineBufferPosition > 0 && LineBufferPosition <= LineBuffer.Length) {
                        --LineBufferPosition;
                        string NextString = LineBuffer.At(LineBuffer.Length - LineBufferPosition);
                        // DebugPrint("Key down. Buffer index = %d, NextString length = %d", BufferIndex, NextString.Length);
                        CopyInto(NextString, Buffer);

                        // redraw line
                        move(0, StringLength(prompt));
                        wclrtoeol(stdscr);
                        addstr(Buffer);

                        refresh();

                        Buffer[NextString.Length] = '\0';
                        BufferIndex = NextString.Length;
                    }
                } else if(Char == KEY_LEFT) {
                    if(BufferIndex > 0) {
                        --BufferIndex;
                        position Cursor = CurrentPosition();
                        --Cursor.X;
                        MoveCursor(Cursor);
                    }
                } else if(Char == KEY_RIGHT) {
                    if(Buffer[BufferIndex] != 0 && BufferIndex < StringLength(Buffer)) {
                        ++BufferIndex;
                        position Position = CurrentPosition();
                        ++Position.X;
                        MoveCursor(Position);
                    }
                } else if(Char == 127 || Char == KEY_BACKSPACE || Char == KEY_DC) {
                    if(BufferIndex > 0) {
                        --BufferIndex;
                        for(int Index = BufferIndex; Index < StringLength(Buffer) && Buffer[Index] != 0; Index++) {
                            Buffer[Index] = Buffer[Index + 1];
                        }
                        position Cursor = CurrentPosition();
                        --Cursor.X;
                        MoveCursor(Cursor);
                        wdelch(stdscr);
                    }
                }
                // TODO: Check for control and special characters
            }
        }

        char* Text = Buffer;

        if(strcmp(Text, "") != 0) {
            // Parse string
            if(strcmp(Text, "quit") == 0 || strcmp(Text, "exit") == 0) {
                break;
            }

            while(IsWhitespace(*Text)) {
                Text++;
            }

            // read num
            int NumDice = atoi(Text);
            if(NumDice == 0) {
                NumDice = 1;
            }

            if(*Text == '-' || *Text == '+') {
                Text++;
            }

            while(IsNumber(*Text)) {
                Text++;
            }

            if(*Text != 'd' && *Text != 'D') {
                if(NumDice > 0) {
                    addstr("Unexpected value. Expected to find d or D after number of dice\n");
                } else {
                    addstr("Unexpected value. Expected to find number, d or D\n");
                }
            } else if(!IsNumber(Text[1])) {
                printw("Expected number after '%c'", *Text);
            } else {
                ++Text;
                int NumSides = atoi(Text);

                int NumDigits = 0;
                while(IsNumber(*Text)) {
                    ++NumDigits;
                    ++Text;
                }

                if(NumSides <= 0) {
                    addstr("Cannot have die with zero or fewer sides or need num sides\n");
                    refresh();
                } else {
                    position WinSize = WindowSize();

                    // 5 = 1 prompt line + 3 lines for stats + 1 line before
                    int NumUsableLines = WinSize.Y - 5;

                    int NumPerLine = 1;

                    // TEMPORARY
                    // TODO: Do the calculus problem that will give this answer, I'm too sleep-deprived to do it now.
                    while((NumDice / NumPerLine) > NumUsableLines) {
                        ++NumPerLine;
                    }

                    if(NumDice > 1) {
                        position CurPos = CurrentPosition();
                        CurPos.Y += 4;
                        MoveCursor(CurPos);
                    }

                    int Total = 0;
                    int Max = 0;
                    int Min = 0x7FFFFFFF;
                    int Num;

                    auto GenerateNum = [&]() {
                        Num = rand() % NumSides + 1;
                        Total += Num;

                        if(Num > Max) {
                            Max = Num;
                        }

                        if(Num < Min) {
                            Min = Num;
                        }
                    };

                    int NumWrittenOnLine = 0;
                    int NumLinesWritten = 0;
                    int Index = 0;

                    char FormatString[100] = "%";
                    snprintf(FormatString + 1, StringLength(FormatString) - 1, "%dd", NumDigits);

                    for(; Index < NumDice && NumLinesWritten < NumUsableLines; ++Index) {
                        GenerateNum();

                        printw(FormatString, Num);

                        ++NumWrittenOnLine;
                        if(NumWrittenOnLine >= NumPerLine || (NumWrittenOnLine * NumDigits) > WinSize.X) {
                            NumWrittenOnLine = 0;
                            ++NumLinesWritten;
                            printw("\n");
                        } else {
                            ++NumWrittenOnLine;
                            printw(" ");
                        }
                    }

                    // out of lines we can write on but continue generating numbers
                    for(; Index < NumDice; ++Index) {
                        GenerateNum();
                    }

                    if(NumDice > 1) {
                        position NewPos = {};
                        NewPos.X = 0;
                        NewPos.Y = 1;
                        MoveCursor(NewPos);
                        printw("Total: %d\n"
                               "Max: %d\n"
                               "Min %d\n\n",
                               Total, Max, Min);
                    }
                } 
            }
        }

        refresh();
    }

    endwin();

    return 0;
}
