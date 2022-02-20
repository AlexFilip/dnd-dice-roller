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

struct position {
    int x;
    int y;
};

position current_position() {
    position result = {};

    getyx(stdscr, result.y, result.x);

    return result;
}

position window_size() {
    position result = {};

    getmaxyx(stdscr, result.y, result.x);

    return result;
}

void move_cursor(position pos) {
    move(pos.y, pos.x);
}

void debug_print(char const* format, ...) {
    position start_pos = current_position();
    position win_size = window_size();

    move(win_size.y - 1, 0);

    char buffer[4096];

    va_list args;
    va_start(args, format);

    vsnprintf(buffer, ArrayLength(buffer), format, args);

    va_end(args);

    wclrtoeol(stdscr);
    wprintw(stdscr, buffer);

    move_cursor(start_pos);
}

char const prompt[] = "> ";

int main() {

    char buffer[100];
    srand(time(NULL));

    char const* DebugStrings[] = {
        "d20",
        "2d12",
        "8d80",
        "d69",
        "d420"
    };

    int DebugStringPos = 0;

    dynamic_array<string> line_buffer = {};
    int line_buffer_loc = 0;

    for(int DebugStringIndex = 0; DebugStringIndex < DebugStringPos; ++DebugStringIndex) {
        string heap_string = CopyOnHeap(StringFromC(DebugStrings[DebugStringIndex]));
        Append(&line_buffer, heap_string);
    }

    initscr();
    raw();
    keypad(stdscr, TRUE);
    noecho();

    int buffer_index = 0;

    for(;;) {
        move(0, 0);
        addstr(prompt);
        refresh();

        buffer_index = 0;
        memset(buffer, 0, ArrayLength(buffer));
        line_buffer_loc = 0;

        bool typed_first_letter = false;
        for(;;) {
            int c = getch();

            if(!typed_first_letter) {
                wclrtoeol(stdscr);
                typed_first_letter = true;
            }

            if(isprint(c)) {
                if(buffer_index < StringLength(buffer)) {
                    char next_letter = (char) c;
                    int start_buffer_index = buffer_index;
                    int cur_index = start_buffer_index;
                    while(cur_index < StringLength(buffer) && next_letter != 0) {
                        char temp = buffer[cur_index];
                        buffer[cur_index] = next_letter;
                        next_letter = temp;
                        ++cur_index;
                    }
                    ++buffer_index;

                    position start_pos = current_position();
                    cur_index = start_buffer_index;
                    while(cur_index < StringLength(buffer) && buffer[cur_index] != 0) {
                        addch(buffer[cur_index]);
                        cur_index++;
                    }

                    ++start_pos.x;
                    move_cursor(start_pos);
                    refresh();
                }
            } else {
                if(c == '\n') {
                    wclrtobot(stdscr);
                    position current_pos = current_position();
                    move(current_pos.y + 1, 0);
                    buffer_index = 0;

                    // debug_print("Command is %s\n", buffer);

                    string heap_string = CopyOnHeap(StringFromC(buffer));
                    Append(&line_buffer, heap_string);

                    break;
                } else if(c == KEY_UP) {
                    if(line_buffer_loc >= 0 && line_buffer_loc < line_buffer.Length) {
                        ++line_buffer_loc;
                        string str = line_buffer.At(line_buffer.Length - line_buffer_loc);
                        // debug_print("Key up. buffer_pos = %d, str length = %d", buffer_index, str.Length);
                        CopyInto(str, buffer);

                        // redraw line
                        move(0, StringLength(prompt));
                        wclrtoeol(stdscr);
                        addstr(buffer);

                        refresh();

                        buffer[str.Length] = '\0';
                        buffer_index = str.Length;
                    }
                } else if(c == KEY_DOWN) {
                    if(line_buffer_loc > 0 && line_buffer_loc <= line_buffer.Length) {
                        --line_buffer_loc;
                        string str = line_buffer.At(line_buffer.Length - line_buffer_loc);
                        // debug_print("Key down. buffer index = %d, str length = %d", buffer_index, str.Length);
                        CopyInto(str, buffer);

                        // redraw line
                        move(0, StringLength(prompt));
                        wclrtoeol(stdscr);
                        addstr(buffer);

                        refresh();

                        buffer[str.Length] = '\0';
                        buffer_index = str.Length;
                    }
                } else if(c == KEY_LEFT) {
                    if(buffer_index > 0) {
                        --buffer_index;
                        position pos = current_position();
                        --pos.x;
                        move_cursor(pos);
                    }
                } else if(c == KEY_RIGHT) {
                    if(buffer[buffer_index] != 0 && buffer_index < StringLength(buffer)) {
                        ++buffer_index;
                        position pos = current_position();
                        ++pos.x;
                        move_cursor(pos);
                    }
                } else if(c == 127 || c == KEY_BACKSPACE || c == KEY_DC) {
                    if(buffer_index > 0) {
                        --buffer_index;
                        int i;
                        for(i = buffer_index; i < StringLength(buffer) && buffer[i] != 0; i++) {
                            buffer[i] = buffer[i + 1];
                        }
                        position cur = current_position();
                        cur.x -= 1;
                        move_cursor(cur);
                        wdelch(stdscr);
                    }
                }
                // TODO: Check for control and special characters
            }
        }

        char* text = buffer;

        if(strcmp(text, "") != 0) {
            // Parse string
            if(strcmp(text, "quit") == 0 || strcmp(text, "exit") == 0) {
                break;
            }

            while(isspace(*text)) {
                text++;
            }

            // read num
            int num_dice = atoi(text);
            if(num_dice == 0) {
                num_dice = 1;
            }

            if(*text == '-' || *text == '+') {
                text++;
            }

            while(isdigit(*text)) {
                text++;
            }

            if(*text != 'd' && *text != 'D') {
                addstr("Unexpected format\n");
                refresh();
            } else {
                text++;
                int num_sides = atoi(text);

                if(num_sides <= 0) {
                    addstr("Cannot have die with zero or fewer sides or need num sides\n");
                    refresh();
                } else {
                    int total = 0;
                    for(int i = 0; i < num_dice; ++i) {
                        int num = rand() % num_sides + 1;
                        total += num;

                        printw("%d\n", num);
                        refresh();
                    }

                    if(num_dice > 1) {
                        printw("\nTOTAL: %d\n", total);
                    }
                } 
            }
        }

        move(0, 0);
        refresh();
    }

    endwin();

    return 0;
}
