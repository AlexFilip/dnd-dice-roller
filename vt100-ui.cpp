/*
  File: vt100-ui.cpp
  Date: 09 June 2022
  Creator: Alexandru Filip
  Notice: (C) Copyright 2022 by Alexandru Filip. All rights reserved.
*/
struct vector2_i {
    int X, Y;
};


enum SpecialCharType : int {
    WindowResized = 255
};

global union {
    int Pipe[2];
    struct {
        int ReadHead;
        int WriteHead;
    };
} SpecialReadPipe; // TODO: Rename

global struct termios   OriginalTermIOs;
global struct sigaction OldResizeAction;

global fd_set ReadFileDescriptors;
global int MaxFileDescriptor;

internal void
SetMaxFileDescriptor(int Descriptor) {
    MaxFileDescriptor = MaxFileDescriptor > Descriptor ? MaxFileDescriptor : Descriptor;
}

internal void
DisableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &OriginalTermIOs);
}

internal void
EnableRawMode() {
    tcgetattr(STDIN_FILENO, &OriginalTermIOs);
    atexit(DisableRawMode);

    struct termios raw = OriginalTermIOs;
    raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    // Timeout for read
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

internal void
SetupSpecialReadPipe() {
    if(SpecialReadPipe.ReadHead == 0 && SpecialReadPipe.WriteHead == 0) {
        // uninitialized
        pipe2(SpecialReadPipe.Pipe, O_NONBLOCK);
    }
}

internal vector2_i
GetWindowSize() {
    vector2_i Result = {};
    struct winsize WindowSize;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &WindowSize) != -1 && WindowSize.ws_col != 0) {
        Result.Y = WindowSize.ws_col;
        Result.X = WindowSize.ws_row;
    }

    return Result;
}

internal void
ResizeSignalHandler(int Signal) {
    int Value = WindowResized;
    write(SpecialReadPipe.WriteHead, &Value, sizeof(Value));
}

internal void
SetResizeSignal() {
    struct sigaction Action = {};
    Action.sa_handler = ResizeSignalHandler;

    sigaction(SIGWINCH, &Action, &OldResizeAction);

    SetupSpecialReadPipe();

    SetMaxFileDescriptor(SpecialReadPipe.ReadHead);

    // NOTE: The docs say that you can do this or use pselect with the SignalSet as the last argument.
    //       I was not able to get this working. After trying both methods the result was the program
    //       waiting until one of the file descriptors read something and only then returning.
    // sigset_t SignalSet;
    // sigemptyset(&SignalSet);
    // sigaddset(&SignalSet, SIGWINCH);

    // sigprocmask(SIG_BLOCK, &SignalSet, NULL);
}

internal void
WriteChar(char C) {
    write(STDOUT_FILENO, &C, 1);
}

internal int
ReadChar() {
    int Result = 0;

    select(STDIN_FILENO + 1, &ReadFileDescriptors, NULL, NULL, NULL);

    vector2_i NewWindowSize = GetWindowSize();
    if(read(SpecialReadPipe.ReadHead, &Result, sizeof(SpecialCharType)) > 0) {
        // Result ok
    } else if(read(STDIN_FILENO, &Result, 1) > 0) {
        // Result ok
    } else {
        printf("READ ERROR!!!");
    }

    return Result;
}

internal void
ClearToEndOfLine() {
    char ClearCommand[] = "\x1b[0K";
    write(STDOUT_FILENO, ClearCommand, sizeof(ClearCommand));
}

internal void
ClearScreen() {
    char ClearCommand[] = "\x1b[2J";
    write(STDOUT_FILENO, ClearCommand, sizeof(ClearCommand));
}

internal void
MoveCursorTo(vector2_i Position) {
    char Buffer[32] = {};
    snprintf(Buffer, sizeof(Buffer), "\x1b[%d;%df", Position.Y, Position.X);
    write(STDOUT_FILENO, Buffer, strlen(Buffer));
}

internal void
MoveCursorToTop() {
    char Buffer[] = "\x1b[1;1f";
    write(STDOUT_FILENO, Buffer, ArrayLength(Buffer) - 1);
}

internal void
MoveCursorByAmountInDirection(int NumPositions, char ForwardChar, char BackwardChar) {
    if(NumPositions != 0) {
        char Char = ForwardChar;
        if(NumPositions < 0) {
            Char = BackwardChar;
            NumPositions = -NumPositions;
        }
        char Buffer[32] = {};
        snprintf(Buffer, sizeof(Buffer) - 1, "\x1b[%d%c", NumPositions, Char);

        int StringLength = strlen(Buffer);
        write(STDOUT_FILENO, Buffer, StringLength);
    }
}

internal void
MoveCursorByX(int NumPositions) {
    MoveCursorByAmountInDirection(NumPositions, 'C', 'D');
}

internal void
MoveCursorByY(int NumPositions) {
    MoveCursorByAmountInDirection(NumPositions, 'B', 'A');
}

internal void
MoveCursorBy(vector2_i Vec) {
    MoveCursorByX(Vec.X);
    MoveCursorByY(Vec.Y);
}

internal void
SaveScreenState() {
    // Save cursor position
    char const SaveCursorString[] = "\x1b" "7";
    write(STDOUT_FILENO, SaveCursorString, sizeof(SaveCursorString) - 1);

    // Save screen contents
    char const SaveString[] = "\x1b[?47h";
    write(STDOUT_FILENO, SaveString, sizeof(SaveString) - 1);
}

internal void
RestoreScreenState() {
    // Restore screen contents
    char const RestoreString[] =  "\x1b[?47l";
    write(STDOUT_FILENO, RestoreString, sizeof(RestoreString) - 1);

    // Restore cursor position
    char const RestoreCursorString[] = "\x1b" "8";
    write(STDOUT_FILENO, RestoreCursorString, sizeof(RestoreCursorString) - 1);
}

internal void
InitVT100UI() {
    FD_ZERO(&ReadFileDescriptors);
    FD_SET(STDIN_FILENO, &ReadFileDescriptors);
    SetMaxFileDescriptor(STDIN_FILENO);

    // FD_ZERO(&WriteFileDescriptors);
    // FD_ZERO(&ExceptionalFileDescriptors);
}


internal void
DebugPrint(char const* Format, ...) {
    // Print at a predictable location
}

