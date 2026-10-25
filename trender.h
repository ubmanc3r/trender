/** tRender v0.3-dev
    release date: 25-10-2026 (https://github.com/ubmanc3r/trender/)
FEATURES:
    - Single-header, no external dependencies
    - Works on Windows, Linux, MacOS (probably)
    - Written in C99 in snake_case/PascalCase notation

    - No hidden state: you own your UI state, the library only draws
    - Full UTF-8 support, including wide (2-cell) CJK characters and emoji
    - Automatic terminal resize handling
    - Keyboard input (Ascii, not Unicode)
    - Clipping: every draw call takes a Rect to clip against
    - Supported color modes: CM_TRUECOLOR, CM_256, CM_16
    - Sub-cell drawing primitives: Braille (2x4) and Halfblock (1x2)
    - Built-in border styles (single, double, rounded, heavy, dashed, ASCII) and custom ones
    - Mixed linear interpolation with many built-in modes and easing
    - Timers, time and frame limiting
**/
#ifndef TRENDER_H
#define TRENDER_H

#ifdef _WIN32
    #include <windows.h>
#else
    #define _POSIX_C_SOURCE 200809L /* the 2008 edition of the POSIX standard (IEEE Standard 1003.1-2008) */
    #include <sys/ioctl.h>
    #include <unistd.h>
    #include <locale.h>
    #include <termios.h>
    #include <time.h>
#endif

#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

/* Defined T_PI since M_PI (math.h) isn't included in the C99 standard */
/* Used only by the built-in interpolation functions */
#define T_PI 3.14159265358979323846

/* Vector2 struct, 2 integer components. Mainly used for terminal cell coordinates */
typedef struct Vector2 { int x, y; } Vector2;
#define V2(x, y) ((Vector2){(x), (y)}) /* Vector2 constructor macro */

/* Rect (rectangle) struct, 2 Vector2 components */
typedef struct Rect { Vector2 pos, size; } Rect;
#define RECT(pos_x, pos_y, size_x, size_y) ((Rect){(V2(pos_x, pos_y)), (V2(size_x, size_y))}) /* Rect constructor macro */

/* BorderType struct, 8 unsigned int components (256bit) */
/* Glyphs in order: top, bottom, left, right, top-left, top-right, bottom-left, bottom-right */
typedef struct BorderType {unsigned int t, b, l, r, tl, tr, bl, br;} BorderType;
/* BorderType constructor macro */
#define BT(t, b, l, r, tl, tr, bl, br) ((BorderType){(t), (b), (l), (r), (tl), (tr), (bl), (br)})

/* Built-in Border Types - using explicit Unicode codepoints */
#define BT_SINGLE  BT(0x2500, 0x2500, 0x2502, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518) /* Single ──││┌┐└┘ (built-in border types) */
#define BT_DOUBLE  BT(0x2550, 0x2550, 0x2551, 0x2551, 0x2554, 0x2557, 0x255A, 0x255D) /* Double ══║║╔╗╚╝ (built-in border types) */
#define BT_ROUNDED BT(0x2500, 0x2500, 0x2502, 0x2502, 0x256D, 0x256E, 0x2570, 0x256F) /* Rounded ──││╭╮╰╯ (built-in border types)  */
#define BT_HEAVY   BT(0x2501, 0x2501, 0x2503, 0x2503, 0x250F, 0x2513, 0x2517, 0x251B) /* Heavy ━━┃┃┏┓┗┛ */
#define BT_DASHED2 BT(0x254C, 0x254C, 0x254E, 0x254E, 0x250C, 0x2510, 0x2514, 0x2518) /* Double Dashed ╌╌╎╎┌┐└┘ (built-in border types) */
#define BT_DASHED3 BT(0x2504, 0x2504, 0x2506, 0x2506, 0x250C, 0x2510, 0x2514, 0x2518) /* Triple Dashed ┄┄┆┆┌┐└┘ (built-in border types) */
#define BT_DASHED4 BT(0x2508, 0x2508, 0x250A, 0x250A, 0x250C, 0x2510, 0x2514, 0x2518) /* Quadruple Dashed ┈┈┊┊┌┐└┘ (built-in border types) */
#define BT_ASCII   BT('-', '-', '|', '|', '+', '+', '+', '+')                         /* ASCII --||++++ (built-in border types) */

/* Color struct, 3 unsigned char components, R8G8B8 (24bit) */
/* Most modern terminals support direct RGB values, otherwise a color will be replaced with the most similar one */
typedef struct Color { unsigned char r, g, b; } Color;
#define C(r, g, b) ((Color){(r), (g), (b)}) /* Color constructor macro */

/* Supported color modes. Set with t_set_color_mode, get with t_get_color_mode. */
/* The internal variable g_color_mode is CM_TRUECOLOR by default. */
typedef enum ColorMode {
    CM_TRUECOLOR, CM_256, CM_16
} ColorMode;

/* Built-in Colors */
#define WHITE     C(255, 255, 255) /* White (built-in colors) */
#define BLACK     C(0,   0,   0)   /* Black (built-in colors) */
#define LIGHTGRAY C(220, 220, 220) /* Light Gray (built-in colors) */
#define GRAY      C(155, 155, 155) /* Gray (built-in colors) */
#define DARKGRAY  C(80,  80,  80)  /* Dark Gray (built-in colors) */
#define RED       C(255, 0,   0)   /* Red (built-in colors) */
#define GREEN     C(0,   255, 0)   /* Green (built-in colors) */
#define BLUE      C(0,   0,   255) /* Blue (built-in colors) */
#define YELLOW    C(255, 255, 0)   /* Yellow (built-in colors) */
#define ORANGE    C(255, 160, 16)  /* Orange (built-in colors) */
#define CYAN      C(0,   255, 255) /* Cyan (built-in colors) */
#define MAGENTA   C(255, 0,   255) /* Magenta (built-in colors) */
#define PINK      C(255, 96,  208) /* Pink (built-in colors) */
#define BROWN     C(160, 128, 96)  /* Brown (built-in colors) */

/* Cell struct, used as a template for custom objects */
typedef struct Cell { unsigned int codepoint; Color fg, bg; } Cell;
#define CELL(codepoint, fg, bg) ((Cell){(codepoint), (fg), (bg)})
#define CELL_EMPTY 0 /* Transparent cell, won't render on top of what already exists on screen */
#define CELL_CONTINUATION 0xFFFFFFFFu /* Continuation of a wide symbol */

/* Present modes. Set with t_set_present_mode, get with t_get_present_mode. */
/* The internal variable g_present_mode is PM_AUTO by default. */
/* PM_AUTO - chooses between PM_FULL and PM_DIFF automatically, based on how many cells changed.  */
/* PM_FULL - redraws every cell each frame. Best when most cells change every frame. */
/* PM_DIFF - redraws only the cells that changed since the previous frame. Best for mostly static screens. */
typedef enum { PM_AUTO, PM_FULL, PM_DIFF } PresentMode;

typedef enum {
    KEY_NONE, KEY_CHAR, KEY_ESC,
    KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
    KEY_ENTER, KEY_BACKSPACE, KEY_TAB
} KeyType;
typedef struct { KeyType type; unsigned int codepoint; } KeyEvent;

typedef struct {
    double start_time;
    double duration_ms;
    bool   running;
} Timer;

static int input_read_byte(unsigned char *out);
KeyEvent t_poll_key(void);

void timer_start(Timer *t, double duration_ms);
float timer_progress(Timer *t);
bool timer_done(Timer *t);

unsigned int cp(const char *utf8_char);

bool t_should_close(void);  /* Returns true once a close has been requested */
void t_request_close(void); /* Set g_should_close to true */
void t_set_color_mode(ColorMode color); /* Set terminal color mode */
ColorMode t_get_color_mode(void);       /* Get terminal color mode */
void t_init(void);          /* Initialize terminal */
double t_time_ms(void);     /* Returns monotonic time in milliseconds (arbitrary start point, use for measuring intervals) */
void t_wait_fps(int target_fps); /* Calls Sleep (Windows) or nanosleep (POSIX) for a fraction of a second (target_fps) */
void t_close(void);         /* Restore original terminal state */
Vector2 t_get_size(void);   /* Returns terminal dimensions as a Vector2 */
Rect t_get_rect(void);      /* Returns a Rect corresponding to the current terminal size */
void t_set_present_mode(PresentMode mode); /* Set terminal present mode */
PresentMode t_get_present_mode(void);      /* Get terminal present mode */
void t_clear(Color fg, Color bg); /* Blanks the entire back buffer to a solid background color */
void t_update(void);        /* Finishes the frame: handles terminal resize and draws the back buffer to the terminal */

/* Mixes linear interpolation with a given preset. Default preset functions begin with ease_. */
/* easing: 0.0 = linear, 1.0 = preset */
static inline float ease_blend(float x, float easing, float (*preset)(float)) { return x + easing * (preset(x) - x); }
static inline float ease_in_quad(float x)      { return x * x; }
static inline float ease_out_quad(float x)     { return 1.0f - (1.0f - x) * (1.0f - x); }
static inline float ease_in_out_quad(float x)  { return x < 0.5f ? 2.0f * x * x : 1.0f - powf(-2.0f * x + 2.0f, 2) / 2.0f; }
static inline float ease_in_cubic(float x)     { return x * x * x; }
static inline float ease_out_cubic(float x)    { return 1.0f - powf((1.0f - x), 3) / 2.0f; }
static inline float ease_in_out_cubic(float x) { return x < 0.5f ? 4.0f * x * x * x: 1.0f - powf(-2.0f * x + 2.0f, 3) / 2.0f; }
static inline float ease_in_sine(float x)      { return 1.0f - cosf((x * (float)T_PI) / 2.0f); }
static inline float ease_out_sine(float x)     { return sinf((x * (float)T_PI) / 2.0f);}
static inline float ease_in_out_sine(float x)  { return -(cosf((float)T_PI * x) - 1.0f) / 2.0f; }

bool cell_visible(Rect clip, Vector2 pos); /* Check if a cell is visible inside a clip and the whole screen */
void t_print(Rect clip, Vector2 pos, Color fg, Color bg, const char *text);       /* Print text */
void t_printf(Rect clip, Vector2 pos, Color fg, Color bg, const char *text, ...); /* Print formatted text */
void t_put(Rect clip, Vector2 pos, Color fg, Color bg, unsigned int codepoint);   /* Put a characher at position */
Cell t_cell_at(Rect clip, Vector2 pos);   /* Get a cell at position (return the left half for the second half of 2-wide symbol) */
void t_recolor(Rect clip, Vector2 pos, Color fg, Color bg);     /* Recolor a cell at position */
void t_recolor_area(Rect clip, Rect area, Color fg, Color bg);  /* Recolor cells in an area */
void t_invert(Rect clip, Vector2 pos);    /* Invert cell colors at position */
void t_invert_area(Rect clip, Rect area); /* Invert cell colors in an area */

void t_line(Rect clip, Vector2 start, Vector2 end, Color fg, Color bg, unsigned int codepoint);                  /* Draw a line*/
void t_rect(Rect clip, Vector2 pos, Vector2 size, Color fg, Color bg, unsigned int codepoint);                   /* Draw rectangle edges */
void t_rect_fill(Rect clip, Vector2 pos, Vector2 size, Color fg, Color bg, unsigned int codepoint);              /* Draw a filled rectangle */
void t_triangle(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg, unsigned int codepoint);      /* Draw triangle edges */
void t_triangle_fill(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg, unsigned int codepoint); /* Draw a filled triangle */

void t_border(Rect clip, Vector2 pos, Vector2 size, Color fg, Color bg, BorderType bordertype); /* Draw a custom-styled border */
void t_cells(Rect clip, Vector2 pos, Vector2 size, const Cell *cells);                          /* Draw a flat array of cells */

void tb_put(Rect clip, Vector2 pos, Color fg, Color bg);                                  /* Draw a braille dot */
void tb_line(Rect clip, Vector2 a, Vector2 b, Color fg, Color bg);                        /* Draw a line (braille) */
void tb_triangle(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg);      /* Draw triangle lines (braille) */
void tb_triangle_fill(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg); /* Draw a filled triangle (braille) */

void th_put(Rect clip, Vector2 pos, Color color);                                         /* Draw a block (halfblock) */
void th_line(Rect clip, Vector2 a, Vector2 b, Color color);                               /* Draw a line (halfblock) */
void th_triangle(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color color);             /* Draw triangle lines (halfblock) */
void th_triangle_fill(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color color);        /* Draw a filled triangle (halfblock) */

#endif /* TRENDER_H */
#ifdef TRENDER_IMPLEMENTATION

static const unsigned char BRAILLE_BIT[4][2] = {
    /* x=0   x=1 */
    { 0x01, 0x08 }, /* y=0 */
    { 0x02, 0x10 }, /* y=1 */
    { 0x04, 0x20 }, /* y=2 */
    { 0x40, 0x80 }, /* y=3 */
};

typedef struct Screen {
    Cell *front, *back;
    Vector2 size;
} Screen;
static Screen g_screen;

static ColorMode g_color_mode = CM_TRUECOLOR;
static PresentMode g_present_mode = PM_AUTO;
static char *outbuf = NULL;
static size_t outcap = 0;
static bool g_closed = false;
static volatile sig_atomic_t g_should_close = 0;
static void handle_sigint(int sig) {
    (void)sig;
    g_should_close = 1;
}

#ifdef _WIN32
static DWORD g_orig_console_mode; /* for STD_OUTPUT_HANDLE, affects t_init/t_close */
static DWORD g_orig_input_mode;   /* for STD_INPUT_HANDLE, affects t_input_init/t_input_close */
static void t_input_init(void) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(h, &g_orig_input_mode);
    DWORD raw = g_orig_input_mode;
    raw &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT |
             ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT);
    raw |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    SetConsoleMode(h, raw);
}
static void t_input_close(void) { SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), g_orig_input_mode); }
static int input_read_byte(unsigned char *out) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    for (;;) {
        DWORD avail = 0;
        if (!GetNumberOfConsoleInputEvents(h, &avail) || avail == 0)
            return 0;   /* Queue is empty */

        INPUT_RECORD rec;
        DWORD read_count;
        if (!ReadConsoleInput(h, &rec, 1, &read_count) || read_count == 0)
            return 0;

        /* Skip everything that is not a keypress */
        if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown && rec.Event.KeyEvent.uChar.AsciiChar != 0) {
            *out = (unsigned char)rec.Event.KeyEvent.uChar.AsciiChar;
            return 1;
        }
    }
}

#else /* POSIX */
static struct termios g_orig_termios;
void t_input_init(void) {
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void t_input_close(void) { tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios); }
static int input_read_byte(unsigned char *out) { return read(STDIN_FILENO, out, 1) == 1; }
#endif

static int codepoint_width(unsigned int cp) {
    /* Common double-width ranges: CJK, Hangul, fullwidth forms, most emoji */
    if ((cp >= 0x1100  && cp <= 0x115F)  ||  /* Hangul Jamo */
        (cp >= 0x2E80  && cp <= 0x303E)  ||  /* CJK radicals, punctuation */
        (cp >= 0x3041  && cp <= 0x33FF)  ||  /* Hiragana, Katakana, CJK symbols */
        (cp >= 0x3400  && cp <= 0x4DBF)  ||  /* CJK extension A */
        (cp >= 0x4E00  && cp <= 0x9FFF)  ||  /* CJK unified ideographs */
        (cp >= 0xA000  && cp <= 0xA4CF)  ||  /* Yi syllables */
        (cp >= 0xAC00  && cp <= 0xD7A3)  ||  /* Hangul syllables */
        (cp >= 0xF900  && cp <= 0xFAFF)  ||  /* CJK compatibility ideographs */
        (cp >= 0xFF00  && cp <= 0xFF60)  ||  /* Fullwidth forms */
        (cp >= 0xFFE0  && cp <= 0xFFE6)  ||
        (cp >= 0x1F300 && cp <= 0x1FAFF) ||  /* Most emoji blocks */
        (cp >= 0x20000 && cp <= 0x3FFFD))    /* CJK extension B+ */
        return 2;

    if (cp == 0) /* NUL — shouldn't normally reach here, but treat as zero-width */
        return 0;

    return 1;
}
static int utf8_decode(const char *s, unsigned int *codepoint) {
    unsigned char c = (unsigned char)s[0];

    if (c < 0x80)            { *codepoint = c; return 1; }
    if ((c & 0xE0) == 0xC0)  { *codepoint = ((c & 0x1F) << 6)  | (s[1] & 0x3F); return 2; }
    if ((c & 0xF0) == 0xE0)  { *codepoint = ((c & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); return 3; }
    if ((c & 0xF8) == 0xF0)  { *codepoint = ((c & 0x07) << 18) | ((s[1] & 0x3F) << 12) | ((s[2] & 0x3F) << 6) | (s[3] & 0x3F); return 4; }

    *codepoint = '?';  /* invalid leading byte, fall back rather than read garbage */
    return 1;
}
static int utf8_encode(unsigned int codepoint, char out[4]) {
    if (codepoint < 0x80) {
        out[0] = (char)codepoint;
        return 1;
    }
    if (codepoint < 0x800) {
        out[0] = (char)(0xC0 | (codepoint >> 6));
        out[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    }
    if (codepoint < 0x10000) {
        out[0] = (char)(0xE0 | (codepoint >> 12));
        out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        out[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (codepoint >> 18));
    out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
    out[3] = (char)(0x80 | (codepoint & 0x3F));
    return 4;
}
unsigned int cp(const char *utf8_char) {
    unsigned int codepoint;
    utf8_decode(utf8_char, &codepoint);
    return codepoint;
}

KeyEvent t_poll_key(void) {
    unsigned char c;
    if (!input_read_byte(&c))
        return (KeyEvent){KEY_NONE};

    if (c == 0x03) { t_request_close(); return (KeyEvent){KEY_NONE}; }
    if (c == 0x1B) {
        unsigned char seq[2];
        if (!input_read_byte(&seq[0])) return (KeyEvent){KEY_ESC};
        if (!input_read_byte(&seq[1])) return (KeyEvent){KEY_ESC};

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return (KeyEvent){KEY_UP};
                case 'B': return (KeyEvent){KEY_DOWN};
                case 'C': return (KeyEvent){KEY_RIGHT};
                case 'D': return (KeyEvent){KEY_LEFT};
            }
        }
        return (KeyEvent){KEY_ESC};
    }
    if (c == '\r' || c == '\n') return (KeyEvent){KEY_ENTER};
    if (c == 0x7F || c == 0x08) return (KeyEvent){KEY_BACKSPACE};
    if (c == '\t')              return (KeyEvent){KEY_TAB};

    if (c >= 0x80) {
        unsigned char buf[4] = {c, 0, 0, 0};
        int extra = (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : 0;
        for (int i = 0; i < extra; ++i)
            input_read_byte(&buf[i+1]);

        unsigned int cp;
        utf8_decode((char*)buf, &cp);
        return (KeyEvent){KEY_CHAR, cp};
    }

    return (KeyEvent){KEY_CHAR, c};
}

void timer_start(Timer *t, double duration_ms) {
    t->start_time = t_time_ms();
    t->duration_ms = duration_ms;
    t->running = true;
}
float timer_progress(Timer *t) {
    if (!t->running) return 1.0f;
    float elapsed = (float)(t_time_ms() - t->start_time);
    float t_progress = elapsed / (float)t->duration_ms;

    if (t_progress >= 1.0f) {
        t->running = false;
        return 1.0f;
    }
    return t_progress;
}
bool timer_done(Timer *t) { return !t->running; }

void t_set_color_mode(ColorMode mode) { g_color_mode = mode; }
ColorMode t_get_color_mode(void) { return g_color_mode; }
static int color_to_256(Color c) {
    if (abs(c.r - c.g) < 8 && abs(c.g - c.b) < 8) {
        if (c.r < 8)   return 16;
        if (c.r > 248) return 231;
        return 232 + (c.r - 8) * 24 / 240;
    }
    return 16 + 36 * (c.r * 5 / 255) + 6 * (c.g * 5 / 255) + (c.b * 5 / 255);
}
static int color_to_16(Color c) {
    static const unsigned char pal[16][3] = {
        {0,0,0},{205,0,0},{0,205,0},{205,205,0},{0,0,238},{205,0,205},{0,205,205},{229,229,229},
        {127,127,127},{255,0,0},{0,255,0},{255,255,0},{92,92,255},{255,0,255},{0,255,255},{255,255,255},
    };
    int best = 0, best_d = 1 << 30;
    for (int i = 0; i < 16; ++i) {
        int dr = c.r - pal[i][0], dg = c.g - pal[i][1], db = c.b - pal[i][2];
        int d = dr*dr + dg*dg + db*db;
        if (d < best_d)
        {
            best_d = d;
            best = i;
        }
    }
    return best;
}
static int write_color_escape(char *out, Color fg, Color bg) {
    switch (g_color_mode) {
        case CM_256:
            return sprintf(out, "\x1b[38;5;%d;48;5;%dm", color_to_256(fg), color_to_256(bg));
        case CM_16:
        {
            int f = color_to_16(fg), b = color_to_16(bg);
            return sprintf(out, "\x1b[%d;%dm", f < 8 ? 30 + f : 90 + f - 8, b < 8 ? 40 + b : 100 + b - 8);
        }
        default:
            return sprintf(out, "\x1b[38;2;%d;%d;%d;48;2;%d;%d;%dm", fg.r, fg.g, fg.b, bg.r, bg.g, bg.b);
    };
}

double t_time_ms(void) {
    #ifdef _WIN32
        static LARGE_INTEGER freq;
        static bool initialized = false;
        LARGE_INTEGER now;
        if (!initialized) {
            QueryPerformanceFrequency(&freq);
            initialized = true;
        }
        QueryPerformanceCounter(&now);
        return ((double)now.QuadPart * 1000 / (double)freq.QuadPart);
    #else
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
    #endif
}
void t_wait_fps(int target_fps) {
    static double last_frame_time = 0;
    double target_ms = 1000.0 / target_fps;
    double now = t_time_ms();
    double elapsed = now - last_frame_time;
    if (elapsed < target_ms) {
    #ifdef _WIN32
            Sleep((DWORD)(target_ms - elapsed));
    #else
            struct timespec req;
            double ms = target_ms - elapsed;
            req.tv_sec  = (time_t)(ms / 1000.0);
            req.tv_nsec = (long)((ms - req.tv_sec * 1000.0) * 1e6);
            nanosleep(&req, NULL);
    #endif
    }
    last_frame_time = t_time_ms();
}
void t_init(void) {
    /* Uses <windows.h> if Windows, <locale.h> if POSIX */
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleMode(h, &g_orig_console_mode);   /* Save console mode to restore it on exit */
        SetConsoleMode(h, g_orig_console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    #else
        setlocale(LC_ALL, "");
    #endif
    printf("\x1b[?1049h\x1b[?25l");  /* Enter alt screen, hide cursor */
    fflush(stdout);

    g_screen.size  = t_get_size();
    int count      = g_screen.size.x * g_screen.size.y;
    g_screen.front = (Cell *)calloc(count, sizeof(Cell));
    g_screen.back  = (Cell *)calloc(count, sizeof(Cell));

    memset(g_screen.front, 0xFF, count * sizeof(Cell));

    t_input_init();
    atexit(t_close);
    signal(SIGINT, handle_sigint);
}
bool t_should_close(void) { return g_should_close != 0; }
void t_request_close(void) { g_should_close = 1; }
void t_close(void) {
    if (g_closed) return;
    g_closed = true;
    t_input_close();

    printf("\x1b[0m\x1b[?25h\x1b[?1049l"); /* Reset colors, show cursor, leave alt screen */
    fflush(stdout);

    #ifdef _WIN32
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleMode(h, g_orig_console_mode);
    #endif

    free(g_screen.front);
    free(g_screen.back);
    free(outbuf);
}
Vector2 t_get_size(void) {
    Vector2 size;
    /* Uses <windows.h> if Windows, <sys/ioctl.h> and <unistd.h> if POSIX */
    #ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        size.x = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        size.y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    #else
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
        size.x = w.ws_col;
        size.y = w.ws_row;
    #endif
    return size;
}
Rect t_get_rect(void) { return (Rect){{0, 0}, g_screen.size}; }
static bool check_resize(void) {
    Vector2 current = t_get_size();
    return (current.x != g_screen.size.x || current.y != g_screen.size.y);
}
static void terminal_resize(void) {
    Vector2 new_size = t_get_size();
    printf("\x1b[2J");
    free(g_screen.front);
    free(g_screen.back);

    int count = new_size.x * new_size.y;
    g_screen.front = (Cell *)calloc(count, sizeof(Cell));
    g_screen.back  = (Cell *)calloc(count, sizeof(Cell));
    g_screen.size  = new_size;

    memset(g_screen.front, 0xFF, count * sizeof(Cell));
}
void t_set_present_mode(PresentMode mode) { g_present_mode = mode; }
PresentMode t_get_present_mode(void) { return g_present_mode; }
static bool ensure_outbuf(void) {
    size_t needed = (size_t)g_screen.size.x * g_screen.size.y * 64 + 64;
    if (needed > outcap) {
        char *p = (char *)realloc(outbuf, needed);
        if (p == NULL) return false;
        outbuf = p;
        outcap = needed;
    }
    return true;
}
static void present_full() {
    if (!ensure_outbuf())
        return;
    size_t len = 0;
    len += sprintf(outbuf + len, "\x1b[H");

    Color last_fg = {0}, last_bg = {0};
    bool color_valid = false;

    for (int y = 0; y < g_screen.size.y; ++y) {
        for (int x = 0; x < g_screen.size.x; ++x) {
            Cell *c = &g_screen.back[y * g_screen.size.x + x];

            if (c->codepoint == CELL_CONTINUATION)
                continue;

            if (!color_valid || c->fg.r != last_fg.r || c->fg.g != last_fg.g || c->fg.b != last_fg.b ||
                               c->bg.r != last_bg.r || c->bg.g != last_bg.g || c->bg.b != last_bg.b) {
                len += write_color_escape(outbuf + len, c->fg, c->bg);
                last_fg = c->fg;
                last_bg = c->bg;
                color_valid = true;
            }

            char bytes[4];
            int n = utf8_encode(c->codepoint ? c->codepoint : ' ', bytes);
            memcpy(outbuf + len, bytes, n);
            len += n;
        }
    }
    fwrite(outbuf, 1, len, stdout);
    fflush(stdout);
}
static void present_diff() {
    if (!ensure_outbuf())
        return;
    size_t len = 0;
    Color last_fg = {0}, last_bg = {0};
    bool color_valid = false;   /*  */
    int cur_x = -1, cur_y = -1; /* Current currsor position. -1 = unknown. */

    int w = g_screen.size.x;
    for (int y = 0; y < g_screen.size.y; ++y) {
        for (int x = 0; x < g_screen.size.x; ++x) {
            int i = y * w + x;
            Cell *b = &g_screen.back[i];
            Cell *f = &g_screen.front[i];

            if (memcmp(b, f, sizeof(Cell)) == 0 || b->codepoint == CELL_CONTINUATION)
                continue;

            if (cur_x != x || cur_y != y)
                len += sprintf(outbuf + len, "\x1b[%d;%dH", y + 1, x + 1);

            if (!color_valid || memcmp(&b->fg, &last_fg, sizeof(Color)) != 0
                             || memcmp(&b->bg, &last_bg, sizeof(Color)) != 0) {
                len += write_color_escape(outbuf + len, b->fg, b->bg);
                last_fg = b->fg; last_bg = b->bg; color_valid = true;
            }

            char bytes[4];
            int n = utf8_encode(b->codepoint ? b->codepoint : ' ', bytes);
            memcpy(outbuf + len, bytes, n);
            len += n;

            cur_x = x + (codepoint_width(b->codepoint) == 2 ? 2 : 1);
            if (cur_x >= w) cur_x = -1;
            cur_y = y;
        }
    }
    if (len) {
        fwrite(outbuf, 1, len, stdout);
        fflush(stdout);
    }
}
static void present(void) {
    int count = g_screen.size.x * g_screen.size.y;
    int changed = 0;
    for (int i = 0; i < count; ++i)
        if (memcmp(&g_screen.back[i], &g_screen.front[i], sizeof(Cell)) != 0)
            changed++;
    if (changed == 0)
        return;
    switch (g_present_mode) {
        case PM_AUTO: {
            if (changed * 3 > count)
                present_full();
            else
                present_diff();
            break;
        }
        case PM_FULL: {
            present_full();
            break;
        }
        case PM_DIFF: {
            present_diff();
            break;
        }
    }
    memcpy(g_screen.front, g_screen.back, count * sizeof(Cell));
}
void t_clear(Color fg, Color bg) {
    int count = g_screen.size.x * g_screen.size.y;
    for (int i = 0; i < count; ++i) {
        g_screen.back[i].codepoint = ' ';
        g_screen.back[i].fg        = fg;
        g_screen.back[i].bg        = bg;
    }
}
void t_update(void) {
    if (check_resize())
        terminal_resize();
    present();
}

static inline int global_to_index(Vector2 global) { return global.y * g_screen.size.x + global.x; }

bool cell_visible(Rect clip, Vector2 pos) {
    if (pos.x < clip.pos.x || pos.x >= clip.pos.x + clip.size.x ||
        pos.y < clip.pos.y || pos.y >= clip.pos.y + clip.size.y)
        return false;
    if (pos.x < 0 || pos.x >= g_screen.size.x || pos.y < 0 || pos.y >= g_screen.size.y)
        return false;
    return true;
}
static void screen_write_cell(Rect clip, Vector2 pos, Color fg, Color bg, unsigned int codepoint) {
    if (!cell_visible(clip, pos))
        return;

    int index = global_to_index(pos);
    Cell *c = &g_screen.back[index];

    if (c->codepoint == CELL_CONTINUATION) {
        if (pos.x > 0)
            g_screen.back[index - 1].codepoint = ' ';   /* левая половина осиротела */
    } else if (codepoint_width(c->codepoint) == 2 && pos.x + 1 < g_screen.size.x) {
        g_screen.back[index + 1].codepoint = ' ';       /* правая осиротела */
    }

    c->fg = fg;
    c->bg = bg;
    c->codepoint = codepoint;
}
static void screen_write_codepoint(Rect clip, Vector2 pos, Color fg, Color bg, unsigned int codepoint) {
    int width = codepoint_width(codepoint);
    screen_write_cell(clip, pos, fg, bg, codepoint);
    if (width == 2)
        screen_write_cell(clip, V2(pos.x+1, pos.y), fg, bg, CELL_CONTINUATION);
}
static void screen_write_text(Rect clip, Vector2 pos, Color fg, Color bg, const char *text) {
    int cx = pos.x;
    int cy = pos.y;
    for (const char *p = text; *p;) {
        if (*p == '\n') {
            cx = pos.x;
            cy++;
            p++;
            continue;
        }
        unsigned int codepoint;
        int consumed = utf8_decode(p, &codepoint);

        screen_write_codepoint(clip, V2(cx, cy), fg, bg, codepoint);

        cx += codepoint_width(codepoint);
        p  += consumed;
    }
}

void t_print(Rect clip, Vector2 pos, Color fg, Color bg, const char *text) {
    screen_write_text(clip, pos, fg, bg, text);
}
void t_printf(Rect clip, Vector2 pos, Color fg, Color bg, const char *text, ...) {
    char buffer[512];
    va_list args;
    va_start(args, text);
    vsnprintf(buffer, sizeof(buffer), text, args);
    va_end(args);

    t_print(clip, pos, fg, bg, buffer);
}
void t_put(Rect clip, Vector2 pos, Color fg, Color bg, unsigned int codepoint) {
    screen_write_codepoint(clip, pos, fg, bg, codepoint);
}
Cell t_cell_at(Rect clip, Vector2 pos) {
    if (!cell_visible(clip, pos))
        return (Cell){ CELL_EMPTY, BLACK, BLACK };

    Cell c = g_screen.back[global_to_index(pos)];
    if (c.codepoint == CELL_CONTINUATION && pos.x > 0)
        return g_screen.back[global_to_index(V2(pos.x - 1, pos.y))];
    return c;
}
static void cell_set_colors(int index, Color fg, Color bg) {
    Cell *c = &g_screen.back[index];
    c->fg = fg;
    c->bg = bg;

    int x = index % g_screen.size.x;
    if (x + 1 < g_screen.size.x && (c+1)->codepoint == CELL_CONTINUATION) {
        (c+1)->fg = fg;
        (c+1)->bg = bg;
    }
}
static int cell_index(Rect clip, Vector2 pos) {
    if (!cell_visible(clip, pos))
        return -1;
    int index = global_to_index(pos);
    if (g_screen.back[index].codepoint == CELL_CONTINUATION && pos.x > 0)
        index--;
    return index;
}
void t_recolor(Rect clip, Vector2 pos, Color fg, Color bg) {
    int index = cell_index(clip, pos);
    if (index < 0) return;
    cell_set_colors(index, fg, bg);
}
void t_recolor_area(Rect clip, Rect area, Color fg, Color bg) {
    for (int y = area.pos.y; y < area.pos.y + area.size.y; ++y) {
        for (int x = area.pos.x; x < area.pos.x + area.size.x; ++x) {
            t_recolor(clip, V2(x, y), fg, bg);
        }
    }
}
void t_invert(Rect clip, Vector2 pos) {
    int index = cell_index(clip, pos);
    if (index < 0) return;
    Cell *c = &g_screen.back[index];
    cell_set_colors(index, c->bg, c->fg);
}
void t_invert_area(Rect clip, Rect area) {
    for (int y = area.pos.y; y < area.pos.y + area.size.y; ++y) {
        for (int x = area.pos.x; x < area.pos.x + area.size.x; ++x) {
            Vector2 pos = V2(x, y);
            if (!cell_visible(clip, pos)) continue;
            if (g_screen.back[global_to_index(pos)].codepoint == CELL_CONTINUATION) continue;
            t_invert(clip, pos);
        }
    }
}

static inline int edge_fn(Vector2 a, Vector2 b, Vector2 p) {
    /* Sign shows at what side of an adge a->b lies a point p */
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}
void t_line(Rect clip, Vector2 start, Vector2 end, Color fg, Color bg, unsigned int codepoint) {
    int dx = abs(end.x - start.x);
    int dy = abs(end.y - start.y);
    int sx = (start.x < end.x) ? 1 : -1;
    int sy = (start.y < end.y) ? 1 : -1;
    int err = dx - dy;

    int x = start.x, y = start.y;
    while (true) {
        t_put(clip, V2(x, y), fg, bg, codepoint);
        if (x == end.x && y == end.y) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}
void t_rect(Rect clip, Vector2 pos, Vector2 size, Color fg, Color bg, unsigned int codepoint) {
    for (int x = pos.x; x < pos.x + size.x; ++x) {
        screen_write_cell(clip, V2(x, pos.y), fg, bg, codepoint);
        screen_write_cell(clip, V2(x, pos.y + size.y - 1), fg, bg, codepoint);
    }
    for (int y = pos.y+1; y < pos.y + size.y - 1; y++) {
        screen_write_cell(clip, V2(pos.x, y), fg, bg, codepoint);
        screen_write_cell(clip, V2(pos.x + size.x - 1, y), fg, bg, codepoint);
    }
}
void t_rect_fill(Rect clip, Vector2 pos, Vector2 size, Color fg, Color bg, unsigned int codepoint) {
    for (int x = pos.x; x < pos.x + size.x; ++x) {
        for (int y = pos.y; y < pos.y + size.y; y++) {
            screen_write_cell(clip, V2(x, y), fg, bg, codepoint);
        }
    }
}
void t_triangle(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg, unsigned int codepoint) {
    t_line(clip, v1, v2, fg, bg, codepoint);
    t_line(clip, v2, v3, fg, bg, codepoint);
    t_line(clip, v3, v1, fg, bg, codepoint);
}
void t_triangle_fill(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg, unsigned int codepoint) {
    if (edge_fn(v1, v2, v3) == 0) {
        t_line(clip, v1, v2, fg, bg, codepoint);
        t_line(clip, v2, v3, fg, bg, codepoint);
        return;
    }
    int min_x = v1.x < v2.x ? (v1.x < v3.x ? v1.x : v3.x) : (v2.x < v3.x ? v2.x : v3.x);
    int max_x = v1.x > v2.x ? (v1.x > v3.x ? v1.x : v3.x) : (v2.x > v3.x ? v2.x : v3.x);
    int min_y = v1.y < v2.y ? (v1.y < v3.y ? v1.y : v3.y) : (v2.y < v3.y ? v2.y : v3.y);
    int max_y = v1.y > v2.y ? (v1.y > v3.y ? v1.y : v3.y) : (v2.y > v3.y ? v2.y : v3.y);
    for (int y = min_y; y <= max_y; ++y) {
        for (int x = min_x; x <= max_x; ++x) {
            Vector2 p = V2(x, y);
            int w0 = edge_fn(v2, v3, p);
            int w1 = edge_fn(v3, v1, p);
            int w2 = edge_fn(v1, v2, p);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0))
                t_put(clip, p, fg, bg, codepoint);
        }
    }
    t_triangle(clip, v1, v2, v3, fg, bg, codepoint);
}

void t_border(Rect clip, Vector2 pos, Vector2 size, Color fg, Color bg, BorderType bt) {
    for (int x = pos.x + 1; x < pos.x + size.x - 1; ++x) {
        screen_write_cell(clip, V2(x, pos.y), fg, bg, bt.t);
        screen_write_cell(clip, V2(x, pos.y + size.y - 1), fg, bg, bt.b);
    }
    for (int y = pos.y + 1; y < pos.y + size.y - 1; y++) {
        screen_write_cell(clip, V2(pos.x, y), fg, bg, bt.l);
        screen_write_cell(clip, V2(pos.x + size.x - 1, y), fg, bg, bt.r);
    }
    screen_write_cell(clip, pos, fg, bg, bt.tl);
    screen_write_cell(clip, V2(pos.x+size.x-1, pos.y), fg, bg, bt.tr);
    screen_write_cell(clip, V2(pos.x, pos.y+size.y-1), fg, bg, bt.bl);
    screen_write_cell(clip, V2(pos.x+size.x-1, pos.y+size.y-1), fg, bg, bt.br);
}
void t_cells(Rect clip, Vector2 pos, Vector2 size, const Cell *cells) {
    for (int y = 0; y < size.y; ++y) {
        for (int x = 0; x < size.x; ++x) {
            const Cell *c = &cells[y * size.x + x];
            if (c->codepoint == CELL_EMPTY) continue;
            screen_write_codepoint(clip, V2(pos.x + x, pos.y + y), c->fg, c->bg, c->codepoint);
        }
    }
}

static inline Vector2 braille_to_cell(Vector2 p, unsigned char *bit) {
    *bit = BRAILLE_BIT[p.y % 4][p.x % 2];
    return V2(p.x / 2, p.y / 4);
}
void tb_put(Rect clip, Vector2 pos, Color fg, Color bg) {
    if (pos.x < 0 || pos.y < 0)
        return;

    unsigned char bit;
    Vector2 cell = braille_to_cell(pos, &bit);

    if (!cell_visible(clip, cell))
        return;

    Cell *c = &g_screen.back[global_to_index(cell)];

    unsigned int mask = 0;
    if (c->codepoint >= 0x2800 && c->codepoint <= 0x28FF)
        mask = c->codepoint - 0x2800;

    c->codepoint = 0x2800 + (mask | bit);
    c->fg = fg;
    c->bg = bg;
}
void tb_line(Rect clip, Vector2 a, Vector2 b, Color fg, Color bg) {
    int dx = abs(b.x - a.x);
    int dy = abs(b.y - a.y);
    int sx = (a.x < b.x) ? 1 : -1;
    int sy = (a.y < b.y) ? 1 : -1;
    int err = dx - dy;

    int x = a.x, y = a.y;
    while (true) {
        tb_put(clip, V2(x, y), fg, bg);
        if (x == b.x && y == b.y) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}
void tb_triangle(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg) {
    tb_line(clip, v1, v2, fg, bg);
    tb_line(clip, v2, v3, fg, bg);
    tb_line(clip, v3, v1, fg, bg);
}
void tb_triangle_fill(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color fg, Color bg) {
    if (edge_fn(v1, v2, v3) == 0) {
        tb_line(clip, v1, v2, fg, bg);
        tb_line(clip, v2, v3, fg, bg);
        return;
    }

    int min_x = v1.x < v2.x ? (v1.x < v3.x ? v1.x : v3.x) : (v2.x < v3.x ? v2.x : v3.x);
    int max_x = v1.x > v2.x ? (v1.x > v3.x ? v1.x : v3.x) : (v2.x > v3.x ? v2.x : v3.x);
    int min_y = v1.y < v2.y ? (v1.y < v3.y ? v1.y : v3.y) : (v2.y < v3.y ? v2.y : v3.y);
    int max_y = v1.y > v2.y ? (v1.y > v3.y ? v1.y : v3.y) : (v2.y > v3.y ? v2.y : v3.y);

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            Vector2 p = V2(x, y);
            int w0 = edge_fn(v2, v3, p);
            int w1 = edge_fn(v3, v1, p);
            int w2 = edge_fn(v1, v2, p);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0))
                tb_put(clip, p, fg, bg);
        }
    }
    tb_triangle(clip, v1, v2, v3, fg, bg);
}
void th_put(Rect clip, Vector2 pos, Color color) {
    if (pos.x < 0 || pos.y < 0)
        return;
    Vector2 cell = V2(pos.x, pos.y / 2);
    if (!cell_visible(clip, cell))
        return;

    Cell *c = &g_screen.back[global_to_index(cell)];

    if (c->codepoint != 0x2580) {
        c->fg = c->bg;
        c->codepoint = 0x2580;
    }
    if (pos.y % 2 == 0) c->fg = color; /* Top half */
    else                c->bg = color; /* Bottom half */
}
void th_line(Rect clip, Vector2 a, Vector2 b, Color color) {
    int dx = abs(b.x - a.x);
    int dy = abs(b.y - a.y);
    int sx = (a.x < b.x) ? 1 : -1;
    int sy = (a.y < b.y) ? 1 : -1;
    int err = dx - dy;

    int x = a.x, y = a.y;
    while (true) {
        th_put(clip, V2(x, y), color);
        if (x == b.x && y == b.y) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}
void th_triangle(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color color) {
    th_line(clip, v1, v2, color);
    th_line(clip, v2, v3, color);
    th_line(clip, v3, v1, color);
}
void th_triangle_fill(Rect clip, Vector2 v1, Vector2 v2, Vector2 v3, Color color) {
    if (edge_fn(v1, v2, v3) == 0) {
        th_line(clip, v1, v2, color);
        th_line(clip, v2, v3, color);
        return;
    }

    int min_x = v1.x < v2.x ? (v1.x < v3.x ? v1.x : v3.x) : (v2.x < v3.x ? v2.x : v3.x);
    int max_x = v1.x > v2.x ? (v1.x > v3.x ? v1.x : v3.x) : (v2.x > v3.x ? v2.x : v3.x);
    int min_y = v1.y < v2.y ? (v1.y < v3.y ? v1.y : v3.y) : (v2.y < v3.y ? v2.y : v3.y);
    int max_y = v1.y > v2.y ? (v1.y > v3.y ? v1.y : v3.y) : (v2.y > v3.y ? v2.y : v3.y);

    for (int y = min_y; y <= max_y; ++y)
    {
        for (int x = min_x; x <= max_x; ++x)
        {
            Vector2 p = V2(x, y);
            int w0 = edge_fn(v2, v3, p);
            int w1 = edge_fn(v3, v1, p);
            int w2 = edge_fn(v1, v2, p);

            if ((w0 >= 0 && w1 >= 0 && w2 >= 0) || (w0 <= 0 && w1 <= 0 && w2 <= 0))
                th_put(clip, p, color);
        }
    }
    th_triangle(clip, v1, v2, v3, color);
}

#endif /* TRENDER_IMPLEMENTATION */
