/*
  File: main.cpp
  Date: 18 February 2022
  Creator: Alexandru Filip
  Notice: (C) Copyright 2022 by Alexandru Filip. All rights reserved.
*/

#include <stdint.h>
#include <stdarg.h>
#include <time.h>
// #include <ctype.h>

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <signal.h>

#include "common_defs.h"
#include "basic_types.h"

#include "common_operations.cpp"
#include "vt100-ui.cpp"
#include "dice-cmd.cpp"
#include "random.cpp"

/*
 * TODO:
 *  - Check format for die rolls, error message if unrecognized
 *  - adding and removing items
 *  - Allow GetToken to treat individual words as strings even if it is not a valid token.
 *  - If the user types something and moves up or down in the line buffer, save the working line into the buffer and copy back when they go past the end of the line buffer
 *  - FIX: After resizing, deleting characters, then pressing enter, it will output 2 lines. The expected output and "Error: Unknown character '"
 *
 *  - advantage, disadvantage, sum/total after dice for advantage, disadvantage, total
 *  - labels on dice. ex. 2d20:attack, which will use "attack" instead of 2d20 in the result labels
 *  - comparison operators: >, <, >=, <= which just say true/false or succeeded/failed (ex. 2d12 > 7 will compare the total of 2d12 to 7)
 *    Q: What should the precedence be on labels vs. comparison operators?
 *  - Distribution information on dice rolls
 *
 *  Eventually (just for fun)
 *  - Math expressions that include dice. Ex. "2 * (2d20 + 4) - 3d8 > 7 - 3d6"
 *  - Functions like min(), max(), sum() to ensure the desired number is used
 *  - Put distributions in as well to study possible rolls
 */

// This will make the program take up the whole screen and restore the contents on exit
#define RunAsApp 0

static char const Prompt[] = "> ";
s32 main() {
    char Buffer[100] = {};
    pcg_random_state RandomState = PCGSeed(time(NULL));
    InitVT100UI();

    dynamic_array<string> CommandHistory = {};
    int_size LineBufferPosition = 0;
    b32 IsRunning = false;

    EnableRawMode();
    EnableMouseTracking();
    // StartResizeHandling();

#if RunAsApp
    SaveScreenState();
    MoveCursorToTop();
    ClearScreen();
#endif

    IsRunning = true;
    while(IsRunning) {
        printf(Prompt);
        fflush(stdout);

        int_size BufferIndex  = 0; // The current position in the buffer
        int_size BufferLength = 0; // The total left of the buffer
        ClearBytes(Buffer, ArrayLength(Buffer));
        LineBufferPosition = 0;

        for(;;) {
            s32 Char = ReadChar();

            if(IsPrintable(Char)) {
                WriteChar(Char);
                if(BufferLength < StringLength(Buffer)) {
                    char NextLetter = (char) Char;
                    int_size StartBufferIndex = BufferIndex;
                    int_size CurrentIndex = StartBufferIndex;
                    while(CurrentIndex < StringLength(Buffer) && NextLetter != 0) {
                        char temp = Buffer[CurrentIndex];
                        Buffer[CurrentIndex] = NextLetter;
                        NextLetter = temp;
                        ++CurrentIndex;
                    }

                    Buffer[CurrentIndex] = '\0';
                    ++BufferIndex;
                    ++BufferLength;

                    int_size NumCharsLeft = BufferLength - BufferIndex;
                    write(STDOUT_FILENO, &Buffer[BufferIndex], NumCharsLeft);
                    MoveCursorByX(-NumCharsLeft);
                }
            } else {
                if(Char == WindowResized) {
                    printf("Window Resized \r\n");
                } else if(Char == '\r' || Char == '\n') {
                    if(BufferLength != 0) {
                        printf("\r\n");

                        string HeapString = CopyOnHeap(StringFromC(Buffer));
                        Append(&CommandHistory, HeapString);

                        break;
                    }
                } else if(IsControlChar(Char)) {
                    if(Char == 0x1b) {
                        // Escape
                        Char = ReadChar();
                        if(Char == '[') {
                            // Arrow keys and maybe other control characters
                            Char = ReadChar();
                            if(Char == 'A') {
                                if(LineBufferPosition >= 0 && LineBufferPosition < CommandHistory.Length) {
                                    if(LineBufferPosition == 0) {
                                        // line currently being typed
                                    }

                                    ++LineBufferPosition;
                                    string PrevString = CommandHistory.At(CommandHistory.Length - LineBufferPosition);
                                    CopyInto(PrevString, Buffer);

                                    int_size StartBufferIndex = BufferIndex;
                                    Buffer[PrevString.Length] = '\0';
                                    BufferIndex = PrevString.Length;
                                    BufferLength = PrevString.Length;

                                    // redraw line
                                    MoveCursorByX(-StartBufferIndex);
                                    ClearToEndOfLine();
                                    write(STDOUT_FILENO, Buffer, BufferIndex);
                                }
                            } else if(Char == 'B') {
                                if(LineBufferPosition >= 0 && LineBufferPosition <= CommandHistory.Length) {
                                    string NextString = {};

                                    if(LineBufferPosition == 0) {
                                        // TODO: Copy working line into buffer
                                    } else {
                                        --LineBufferPosition;
                                        NextString = CommandHistory.At(CommandHistory.Length - LineBufferPosition);
                                    }

                                    CopyInto(NextString, Buffer);

                                    int_size StartBufferIndex = BufferIndex;
                                    Buffer[NextString.Length] = '\0';
                                    BufferIndex  = NextString.Length;
                                    BufferLength = NextString.Length;

                                    // redraw line
                                    MoveCursorByX(-StartBufferIndex);
                                    ClearToEndOfLine();
                                    write(STDOUT_FILENO, Buffer, BufferIndex);
                                }
                            } else if(Char == 'C') {
                                if(Buffer[BufferIndex] != 0 && BufferIndex < BufferLength) {
                                    ++BufferIndex;
                                    // Move cursor right
                                    char MoveRight[] = { '\x1b', '[', 'C' };
                                    write(STDOUT_FILENO, MoveRight, ArrayLength(MoveRight));
                                }
                            } else if(Char == 'D') {
                                if(BufferIndex > 0) {
                                    --BufferIndex;
                                    // Move cursor left
                                    char MoveLeft[] = { '\x1b', '[', 'D' };
                                    write(STDOUT_FILENO, MoveLeft, ArrayLength(MoveLeft));
                                }
                            }
                        } else {
                            printf("\r\nUnknown escape sequence: ");
                            while(read(STDIN_FILENO, &Char, 1) != 0) {
                                printf("%c", Char);
                            }
                            printf("\r\n");
                        }
                    } else if(Char >= 0 && Char <= 26) {
                        // Ctrl-Space (0) and Ctrl-{A through Z} not including J (line-feed) and M (carriage-return)
                        // char KeyName[] = "space";
                        // if(Char != 0) {
                        //     KeyName[0] = Char + 'a' - 1;
                        //     KeyName[1] = 0;
                        // }
                        // printf("\r\nCtrl-%s\r\n", KeyName);
                        if(Char == 3) {
                            // Ctrl-C
                            MoveCursorByX(-BufferIndex);
                            BufferIndex = 0;
                            BufferLength = 0;
                            ClearToEndOfLine();
                            Buffer[BufferIndex] = 0;
                        } else if(Char == 4) {
                            if(BufferLength == 0) {
                                exit(0);
                            }
                        }
                    } else if (Char == 127) {
                        // Backspace
                        if(BufferIndex > 0) {
                            MoveCursorByX(-1);
                            --BufferIndex;

                            for(int_size Index = BufferIndex; Index < BufferLength; ++Index) {
                                Buffer[Index] = Buffer[Index + 1];
                            }

                            Buffer[BufferLength] = 0;
                            --BufferLength;

                            write(STDOUT_FILENO, &Buffer[BufferIndex], BufferLength - BufferIndex);
                            ClearToEndOfLine();
                            MoveCursorByX(BufferIndex - BufferLength);
                        }
                    } else {
                        printf("\r\nOther control char [%d]\r\n", Char);
                    }
                }
            }
        }

        char* Text = Buffer;

        tokenizer Tokenizer = {};
        Tokenizer.At = Buffer;
        Tokenizer.End = Buffer + BufferLength;
        *(Tokenizer.End) = '\0';

        b32 IsReading = true;
        while(IsReading) {
            // TODO: Replace with
            //   - Read line (expression)
            //   - Evaluate line (new expression or value structs)

            token CurrentToken = GetToken(&Tokenizer);

            if(CurrentToken.Type == TokenTypeEndOfStream) {
                IsReading = false;
            } else if(CurrentToken.Type == TokenTypeDice) {
                s32 Total = 0;
                s32 Max   = 0;
                s32 Min   = 0x7FFFFFFF;

                for(int_size Index = 0; Index < CurrentToken.Dice.Count; ++Index) {
                    s32 Num = NextRandom(&RandomState) % CurrentToken.Dice.NumSides + 1;

                    printf("%d  ", Num);
                    Total += Num;

                    if(Num > Max) {
                        Max = Num;
                    }

                    if(Num < Min) {
                        Min = Num;
                    }
                }

                printf("\r\n");
                if(CurrentToken.Dice.Count != 1) {
                    printf("  Total: %d\r\n"
                           "  Max: %d\r\n"
                           "  Min: %d\r\n\r\n",
                           Total, Max, Min);
                }

            } else if(CurrentToken.Type == TokenTypeIdentifier) {
                if(StringsEqual(CurrentToken.Identifier, String("quit")) || StringsEqual(CurrentToken.Identifier, String("exit"))) {
                    IsRunning = false;
                    IsReading = false;
                } else {
                    printf("Error: '%.*s' is not a valid command\r\n", StringAsArgs(CurrentToken.Identifier));
                }
            } else if(CurrentToken.Type == TokenTypeInt) {
                printf("%d\r\n", CurrentToken.Number);
            } else if(CurrentToken.Type == TokenTypeString) {
                printf("Found string: \"%.*s\"\r\n", StringAsArgs(CurrentToken.String));
            } else if(CurrentToken.Type == TokenTypeError) {
                printf("Error: %.*s\r\n", StringAsArgs(CurrentToken.ErrorMessage));
                IsReading = false;
            } else if(CurrentToken.Type == TokenTypeNone) {
                printf("Error: Received token type = None\r\n");
                IsReading = false;
            }
        }
    }

#if RunAsApp
    RestoreScreenState();
#endif

    return 0;
}
