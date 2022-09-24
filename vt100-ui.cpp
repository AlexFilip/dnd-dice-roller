/*
  File: vt100-ui.cpp
  Date: 09 June 2022
  Creator: Alexandru Filip
  Notice: (C) Copyright 2022 by Alexandru Filip. All rights reserved.
*/
struct vector2_i {
    s32 X, Y;
};

enum SpecialCharType : s32 {
    WindowResized = 256
};

struct pipe_fds {
    s32 ReadHead;
    s32 WriteHead;
};

global pipe_fds InternalMessagePipe;
global b32 SpecialReadPipeExists;

global struct termios   OriginalTermIOs;
global struct sigaction OldResizeAction;

global s32 EPollFileDescriptor;
global struct epoll_event EPollEventBuffer[16];
global s32 NumEPollEvents;
global s32 EPollBufferIndex;
global s32 NumRegisteredEPollFDs;

internal void
RegisterFDForEPollRead(s32 FileDescriptor) {
    if(NumRegisteredEPollFDs < ArrayLength(EPollEventBuffer)) {
        struct epoll_event EventData = {};
        EventData.events = EPOLLIN;
        EventData.data.fd = FileDescriptor;

        epoll_ctl(EPollFileDescriptor, EPOLL_CTL_ADD, FileDescriptor, &EventData);
        NumRegisteredEPollFDs += 1;
    }
}

internal void
UnRegisterFDForEPollRead(s32 FileDescriptor) {
    // Assumes FileDescriptor is registered since there is no way to check
    if(NumRegisteredEPollFDs >= 0) {
        struct epoll_event EventData = {};
        EventData.data.fd = FileDescriptor;

        epoll_ctl(EPollFileDescriptor, EPOLL_CTL_DEL, FileDescriptor, &EventData);
        NumRegisteredEPollFDs -= 1;
    }
}

internal s32
NextEPollFD() {
    s32 Result = 0;
    struct epoll_event EventData = EPollEventBuffer[EPollBufferIndex];

    EPollBufferIndex++;
    NumEPollEvents -= 1;

    Result = EventData.data.fd;

    return Result;
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
EnableMouseTracking() {
    char MouseTracking[] = "\x1b[1000h";
    write(STDOUT_FILENO, MouseTracking, sizeof(MouseTracking));

    char DecimalMouseTracking[] = "\x1b[1006h";
    write(STDOUT_FILENO, DecimalMouseTracking, sizeof(DecimalMouseTracking));
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
ResizeSignalHandler(s32 Signal) {
    s32 Value = WindowResized;
    write(InternalMessagePipe.WriteHead, &Value, sizeof(Value));
}

internal pipe_fds
CreatePipe(b32 Blocking = false) {
    union {
        s32 Pipe[2];
        pipe_fds FileDescriptors;
    } Result = {};

    // TODO: Look into O_DIRECT for "packet mode"
    s32 Flags = 0;
    if(!Blocking) {
        Flags |= O_NONBLOCK;
    }

    pipe2(Result.Pipe, Flags);
    SpecialReadPipeExists = true;

    return Result.FileDescriptors;
}

internal void
DestroyPipe(pipe_fds Pipe) {
    close(Pipe.ReadHead);
    close(Pipe.WriteHead);
}

internal void
StartResizeHandling() {
    struct sigaction Action = {};
    Action.sa_handler = ResizeSignalHandler;

    sigaction(SIGWINCH, &Action, &OldResizeAction);

    if(!SpecialReadPipeExists) {
        // uninitialized
        InternalMessagePipe = CreatePipe();
        SpecialReadPipeExists = true;
    }

    RegisterFDForEPollRead(InternalMessagePipe.ReadHead);
    // SetMaxFileDescriptor(InternalMessagePipe.ReadHead);
    // FD_SET(InternalMessagePipe.ReadHead, &ReadFileDescriptors);

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

internal s32
ReadChar() {
    s32 Result = 0;
    if(NumEPollEvents <= 0) {
        // TODO: Replace select() with poll() or epoll()

        // NOTE: When resizing, this function gets unblocked, not by writing
        // to InternalMessagePipe, but by SIGWINCH triggering its signal handler.
        // select(MaxFileDescriptor + 1, &ReadFileDescriptors, NULL, NULL, NULL);
        NumEPollEvents = epoll_wait(EPollFileDescriptor, EPollEventBuffer, ArrayLength(EPollEventBuffer), -1);
        EPollBufferIndex = 0;
    }

    s32 FileDescriptor = NextEPollFD();
    if(SpecialReadPipeExists && FileDescriptor == InternalMessagePipe.ReadHead) {
        // printf("\r\nInternal message pipe\r\n");
        if(read(InternalMessagePipe.ReadHead, &Result, sizeof(SpecialCharType)) > 0) {
            // TODO: Allow for the ability to send extra info in this special pipe without using switch-case statement here
        } else {
            // TODO: Error
        }
    } else if(FileDescriptor == STDIN_FILENO) {
        // printf("\r\nstdin read\r\n");
        if(read(STDIN_FILENO, &Result, 1) > 0){
            // ???
        } else {
            // TODO: Error
        }
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
    write(STDOUT_FILENO, Buffer, CStringLength(Buffer));
}

internal void
MoveCursorToTop() {
    char Buffer[] = "\x1b[1;1f";
    write(STDOUT_FILENO, Buffer, ArrayLength(Buffer) - 1);
}

internal void
MoveCursorByAmountInDirection(s32 NumPositions, char ForwardChar, char BackwardChar) {
    if(NumPositions != 0) {
        char Char = ForwardChar;
        if(NumPositions < 0) {
            Char = BackwardChar;
            NumPositions = -NumPositions;
        }
        char Buffer[32] = {};
        snprintf(Buffer, sizeof(Buffer) - 1, "\x1b[%d%c", NumPositions, Char);

        s32 StringLength = CStringLength(Buffer);
        write(STDOUT_FILENO, Buffer, StringLength);
    }
}

internal void
MoveCursorByX(s32 NumPositions) {
    MoveCursorByAmountInDirection(NumPositions, 'C', 'D');
}

internal void
MoveCursorByY(s32 NumPositions) {
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
    EPollFileDescriptor = epoll_create(1); // Argument is ignored
    RegisterFDForEPollRead(STDIN_FILENO);
}


internal void
DebugPrint(char const* Format, ...) {
    // Print at a predictable location
}

