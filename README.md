# tRender

Single-header terminal rendering library in c99. No dependencies, no hidden state: you own your UI state, the library only draws.

> **Status: v0.3-dev.** The API may change between 0.x versions.

## Features
- Single header, no external dependencies
- Windows, Linux, macOS (macOS untested)
- Full UTF-8 output, including wide (2-cell) CJK characters and emoji
- Clipping: every draw call takes a `Rect` to clip against
- Color modes: truecolor, 256, 16
- Sub-cell drawing: Braille (2x4) and Halfblock (1x2) points, lines, triangles
- Primitives: text, lines, rectangles, triangles, borders, cell arrays
- Built-in border styles (single, double, rounded, heavy, dashed, ASCII) and custom ones
- Interpolation with easing presets, timers, frame limiting
- Differential rendering (only changed cells are redrawn)
- Automatic terminal resize handling
- Keyboard input (ASCII keys, arrows, Enter, Tab, Backspace)

## Quick start
Copy `trender.h` into your project. Define the implementation macro in **exactly one** `.c` file:
```c
#define TRENDER_IMPLEMENTATION
#include "trender.h"

int main(void) {
    t_init();
    while (!t_should_close()) {
        KeyEvent key = t_poll_key();
        if (key.type == KEY_CHAR && key.codepoint == 'q')
            t_request_close();

        Rect screen = t_get_rect();
        t_clear(WHITE, BLACK);
        t_print(screen, V2(3, 1), GREEN, BLACK, "Hello, tRender!");
        t_border(screen, V2(1, 0), V2(19, 3), WHITE, BLACK, BT_ROUNDED);

        t_update();
        t_wait_fps(60);
    }
    t_close();
    return 0;
}
```
Build:
```
gcc -std=c99 main.c -o main -lm
```
Other files that only need the API include `trender.h` without the macro.

## Design notes
- Keyboard input is ASCII only on Windows (no Unicode input yet)
- No mouse input yet
- No text styles (bold, underline, italic) yet
- Not tested on MacOS
- CM_256 and CM_16 are lightly tested

## License
CC BY 4.0, see `LICENSE`. Copyright (c) 2026 ubmanc3r.

