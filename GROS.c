// RVM OS — Console GUI (Windows) v3
// Clickable desktop icons + "wallpaper" + start menu + animated windows
// Build: cl /O2 RVMOS_Console_GUI_v3.c /Fe:RVMOS.exe /link user32.lib gdi32.lib

#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

// ─────────────────────────────────────────────────────────────────────────────
// Config
// ─────────────────────────────────────────────────────────────────────────────
#define SCREEN_W  120
#define SCREEN_H   35
#define MAX_WIN     8
#define MAX_ICONS  16
#define TITLEBAR_H  1
#define TASKBAR_H   1
#define FPS        60

// Color helpers (console attributes)
#define COL_BG      0
#define COL_FG      (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)
#define COL_DIM     (FOREGROUND_INTENSITY)
#define COL_HI      (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define COL_BTN     (FOREGROUND_BLUE|FOREGROUND_INTENSITY)
#define COL_WARN    (FOREGROUND_RED|FOREGROUND_INTENSITY)
#define COL_OK      (FOREGROUND_GREEN|FOREGROUND_INTENSITY)
#define COL_ACCENT  (FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_INTENSITY)

// Box-drawing shortcuts
#define TL 0x250C
#define TR 0x2510
#define BL 0x2514
#define BR 0x2518
#define HL 0x2500
#define VL 0x2502

// ─────────────────────────────────────────────────────────────────────────────
// Types
// ─────────────────────────────────────────────────────────────────────────────

typedef struct { short x, y, w, h; } Rect;

typedef enum { APP_NONE=0, APP_FILES, APP_EDITOR, APP_CALC } AppKind;

typedef struct {
    bool    used;
    int     id;
    Rect    r;
    wchar_t title[64];
    AppKind app;
    bool    dragging;
    short   drag_dx, drag_dy;
    float   open_t;     // 0..1 animation progress
    bool    closing;
    bool    focused;
} Window;

typedef struct {
    int x, y;         // top-left of icon tile
    int w, h;         // clickable rect
    wchar_t label[24];
    AppKind app;
} Icon;

// ─────────────────────────────────────────────────────────────────────────────
// Globals
// ─────────────────────────────────────────────────────────────────────────────

static HANDLE g_out;
static HANDLE g_in;
static SMALL_RECT g_region;
static CHAR_INFO *g_buf_front = NULL;
static CHAR_INFO *g_buf_back  = NULL;
static int g_sw = SCREEN_W, g_sh = SCREEN_H;
static Window g_windows[MAX_WIN];
static int g_zorder[MAX_WIN];
static int g_win_count = 0;
static bool g_show_start = false;
static DWORD g_last_tick = 0;
static Icon g_icons[MAX_ICONS];
static int g_icon_count = 0;

// ─────────────────────────────────────────────────────────────────────────────
// Utils
// ─────────────────────────────────────────────────────────────────────────────



static inline int idx(int x,int y){ return y*g_sw + x; }

static void clear_buf(CHAR_INFO* b, WORD attr, wchar_t ch){
    for(int i=0;i<g_sw*g_sh;i++){ b[i].Attributes = attr; b[i].Char.UnicodeChar = ch; }
}

static void swap_buffers(){ CHAR_INFO* t=g_buf_front; g_buf_front=g_buf_back; g_buf_back=t; }

static void present(){
    g_region.Left=0; g_region.Top=0; g_region.Right=g_sw-1; g_region.Bottom=g_sh-1;
    WriteConsoleOutputW(g_out, g_buf_front, (COORD){(SHORT)g_sw,(SHORT)g_sh}, (COORD){0,0}, &g_region);
}

static void putc_xy(CHAR_INFO* b,int x,int y,wchar_t ch,WORD attr){
    if(x<0||y<0||x>=g_sw||y>=g_sh) return; int i=idx(x,y); b[i].Char.UnicodeChar=ch; b[i].Attributes=attr; }

static void text(CHAR_INFO* b,int x,int y,WORD attr,const wchar_t* s){
    for(int i=0;s[i]&&x+i<g_sw;i++) putc_xy(b,x+i,y,s[i],attr);
}

static void hline(CHAR_INFO* b,int x,int y,int w,WORD attr,wchar_t ch){
    for(int i=0;i<w;i++) putc_xy(b,x+i,y,ch,attr);
}

static void rect_fill(CHAR_INFO* b, Rect r, WORD attr, wchar_t ch){
    for(int yy=0; yy<r.h; yy++) hline(b, r.x, r.y+yy, r.w, attr, ch);
}

static void rect_clip(Rect* r){
    if(r->x<0){ r->w+=r->x; r->x=0; }
    if(r->y<0){ r->h+=r->y; r->y=0; }
    if(r->x+r->w>g_sw) r->w=g_sw-r->x;
    if(r->y+r->h>g_sh) r->h=g_sh-r->y;
    if(r->w<0) r->w=0; if(r->h<0) r->h=0;
}

static void draw_box(CHAR_INFO* b, Rect r, WORD attr){
    if(r.w<2||r.h<2) return;
    putc_xy(b,r.x, r.y, TL, attr); putc_xy(b,r.x+r.w-1, r.y, TR, attr);
    putc_xy(b,r.x, r.y+r.h-1, BL, attr); putc_xy(b,r.x+r.w-1, r.y+r.h-1, BR, attr);
    hline(b, r.x+1, r.y, r.w-2, attr, HL);
    hline(b, r.x+1, r.y+r.h-1, r.w-2, attr, HL);
    for(int yy=1; yy<r.h-1; yy++){ putc_xy(b,r.x, r.y+yy, VL, attr); putc_xy(b,r.x+r.w-1, r.y+yy, VL, attr);}    
}

static float clamp01(float x){ if(x<0) return 0; if(x>1) return 1; return x; }
static float ease_out_cubic(float t){ t=clamp01(t); t=1.0f-powf(1.0f-t,3.0f); return t; }

static bool point_in(Rect r,int x,int y){ return x>=r.x && y>=r.y && x<r.x+r.w && y<r.y+r.h; }

// ─────────────────────────────────────────────────────────────────────────────
// Window management
// ─────────────────────────────────────────────────────────────────────────────

static void focus_window(int zi){
    int id = g_zorder[zi];
    for(int i=zi;i<g_win_count-1;i++) g_zorder[i]=g_zorder[i+1];
    g_zorder[g_win_count-1]=id;
    for(int i=0;i<g_win_count;i++) g_windows[g_zorder[i]].focused = (i==g_win_count-1);
}

static void close_window(int id){ if(!g_windows[id].used) return; g_windows[id].closing=true; }

static int create_window(int x,int y,int w,int h,const wchar_t* title,AppKind app){
    for(int i=0;i<MAX_WIN;i++) if(!g_windows[i].used){
        Window* win=&g_windows[i];
        *win=(Window){0};
        win->used=true; win->id=i; win->r=(Rect){x,y,w,h};
        wcsncpy(win->title,title,63); win->app=app; win->open_t=0.0f;
        g_zorder[g_win_count++]=i; focus_window(g_win_count-1);
        return i;
    }
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// App content renderers
// ─────────────────────────────────────────────────────────────────────────────

void start_GROS();

static void app_files(CHAR_INFO* b, Window* w){
    int x=w->r.x+2, y=w->r.y+2; WORD a=COL_HI;
    text(b,x,y++,a,L"C:\\");
    text(b,x,y++,a,L"Windows\\");
    text(b,x,y++,a,L"Program Files\\");
    text(b,x,y++,a,L"Users\\");
}

static void app_editor(CHAR_INFO* b, Window* w){
    int x=w->r.x+2, y=w->r.y+2; WORD a=COL_HI;
    text(b,x,y++,a,L"Text Editor — demo");
    text(b,x,y++,COL_DIM,L"Type: Start ▸ → Editor, drag windows, click × to close.");
}

static void app_calc(CHAR_INFO* b, Window* w){
    int x=w->r.x+2, y=w->r.y+2; WORD a=COL_HI;
    text(b,x,y++,a,L"Calculator");
    text(b,x,y++,a,L"┌───────┐");
    text(b,x,y++,a,L"│  42   │");
    text(b,x,y++,a,L"└───────┘");
    text(b,x,y++,COL_DIM,L"7 8 9  +");
    text(b,x,y++,COL_DIM,L"4 5 6  −");
    text(b,x,y++,COL_DIM,L"1 2 3  ×");
    text(b,x,y++,COL_DIM,L"0 . =  ÷");
}

static void render_app(CHAR_INFO* b, Window* w){
    switch(w->app){
        case APP_FILES: app_files(b,w); break;
        case APP_EDITOR: app_editor(b,w); break;
        case APP_CALC: app_calc(b,w); break;
        default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Icons (desktop shortcuts)
// ─────────────────────────────────────────────────────────────────────────────

static void add_icon(int x,int y,const wchar_t* label,AppKind app){
    if(g_icon_count>=MAX_ICONS) return;
    Icon *ic=&g_icons[g_icon_count++];
    ic->x=x; ic->y=y; ic->w=8; ic->h=3; // 1 line glyph + 2 lines label area
    wcsncpy(ic->label,label,23); ic->label[23]=0; ic->app=app;
}

static void init_icons(){
    g_icon_count=0;
    add_icon(3,3,L"🗀 Files",  APP_FILES);
    add_icon(3,7,L"🖹 Editor", APP_EDITOR);
    add_icon(3,11,L"⍰ Calc",   APP_CALC);
}

static void draw_icons(CHAR_INFO* b){
    for(int i=0;i<g_icon_count;i++){
        Icon* ic=&g_icons[i];
        text(b, ic->x, ic->y, (WORD)(COL_HI), ic->label);
        hline(b, ic->x, ic->y+1, (int)wcslen(ic->label), (WORD)COL_DIM, L' ');
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Draw window with chrome + animation
// ─────────────────────────────────────────────────────────────────────────────

static void draw_window(CHAR_INFO* b, Window* w){
    float t = w->open_t; if(w->closing) t = 1.0f - t; t = ease_out_cubic(t);
    int cx = w->r.x + w->r.w/2; int cy = w->r.y + w->r.h/2;
    int halfw = (int)(w->r.w * t * 0.5f);
    int halfh = (int)(w->r.h * t * 0.5f);
    Rect rr = { cx-halfw, cy-halfh, halfw*2, halfh*2 };
    rect_clip(&rr);

    rect_fill(b, rr, (WORD)(COL_BG | (w->focused? COL_ACCENT: COL_DIM)), L' ');

    if(rr.h>=1){
        int tb_y = rr.y; hline(b, rr.x, tb_y, rr.w, (WORD)(BACKGROUND_BLUE|COL_HI), L' ');
        int tx = rr.x+2; int maxw = rr.w-8; if(maxw<0) maxw=0; wchar_t tmp[64]; wcsncpy(tmp,w->title,63); tmp[63]=0;
        if((int)wcslen(tmp)>maxw){ tmp[maxw]=0; }
        text(b, tx, tb_y, (WORD)(BACKGROUND_BLUE|COL_HI), tmp);
        text(b, rr.x+rr.w-7, tb_y, (WORD)(BACKGROUND_BLUE|COL_HI), L"  – □ × ");
    }

    Rect cr = { rr.x+1, rr.y+1, rr.w-2, rr.h-2 }; rect_clip(&cr);
    if(cr.w>0 && cr.h>0){
        rect_fill(b, cr, (WORD)(COL_BG|COL_FG), L' ');
        draw_box(b, (Rect){cr.x,cr.y,cr.w,cr.h}, (WORD)COL_DIM);
        Rect inner = { cr.x+1, cr.y+1, cr.w-2, cr.h-2 }; rect_clip(&inner);
        if(inner.w>0 && inner.h>0) render_app(b,w);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Desktop, taskbar, start menu
// ─────────────────────────────────────────────────────────────────────────────

static void draw_wallpaper(CHAR_INFO* b){
    for(int y=0;y<g_sh;y++){
        for(int x=0;x<g_sw;x++){
            int v = (x + y/2) % 4;
            wchar_t ch = (v==0? L' ' : v==1? L'·' : v==2? L'∙' : L'•');
            WORD at = (WORD)(COL_BG | ((v>=2)? COL_DIM : COL_FG));
            putc_xy(b,x,y,ch,at);
        }
    }
    const wchar_t* name=L"RVM OS";
    int nx=(g_sw-(int)wcslen(name))/2; int ny=g_sh/3;
    text(b,nx+1,ny+1,(WORD)COL_DIM,name);
    text(b,nx,ny,(WORD)COL_HI,name);
}

static void draw_desktop(CHAR_INFO* b){
    draw_wallpaper(b);
    draw_icons(b);
}

static void draw_taskbar(CHAR_INFO* b){
    int y=g_sh-1; hline(b,0,y,g_sw,(WORD)(BACKGROUND_BLUE|COL_HI),L' ');
    text(b,2,y,(WORD)(BACKGROUND_BLUE|COL_HI),L"Start ▸");

    int x=12;
    for(int i=0;i<g_win_count;i++){
        Window* w=&g_windows[g_zorder[i]];
        int ww=(int)wcslen(w->title)+4; if(x+ww>g_sw-16) break;
        WORD a=(WORD)(BACKGROUND_BLUE| (w->focused? COL_ACCENT: COL_HI));
        hline(b,x,y,ww,a,L' '); text(b,x+2,y,a,w->title); x+=ww+1;
    }

    time_t t = time(NULL);
    struct tm tm;
#ifdef _MSC_VER
    localtime_s(&tm, &t);
#else
    struct tm *ptm = localtime(&t); if(ptm) tm = *ptm;
#endif
    wchar_t cbuf[32]; wcsftime(cbuf,32,L"%H:%M",&tm);
    int cw=(int)wcslen(cbuf); text(b,g_sw-cw-2,y,(WORD)(BACKGROUND_BLUE|COL_HI),cbuf);
}

static void draw_start_menu(CHAR_INFO* b){
    if(!g_show_start) return; int w=28,h=10,x=0,y=g_sh-h-1; Rect r={x,y,w,h};
    rect_fill(b,r,(WORD)(BACKGROUND_BLUE|COL_HI),L' ');
    draw_box(b,r,(WORD)COL_HI);
    text(b,x+2,y+1,(WORD)(BACKGROUND_BLUE|COL_HI),L"RVM OS");
    text(b,x+2,y+3,(WORD)(BACKGROUND_BLUE|COL_HI),L"1) Open File Manager");
    text(b,x+2,y+4,(WORD)(BACKGROUND_BLUE|COL_HI),L"2) Open Text Editor");
    text(b,x+2,y+5,(WORD)(BACKGROUND_BLUE|COL_HI),L"3) Open Calculator");
    text(b,x+2,y+7,(WORD)(BACKGROUND_BLUE|COL_HI),L"Esc) Close menu");
}

// ─────────────────────────────────────────────────────────────────────────────
// Input handling
// ─────────────────────────────────────────────────────────────────────────────

static void open_app_from_icon(AppKind app){
    switch(app){
        case APP_FILES:  create_window(18,4,44,14,L"File Manager",APP_FILES); break;
        case APP_EDITOR: create_window(24,6,48,16,L"Text Editor",APP_EDITOR); break;
        case APP_CALC:   create_window(30,8,40,14,L"Calculator",APP_CALC); break;
        default: break;
    }
}

static void handle_keyboard(KEY_EVENT_RECORD k){
    if(!k.bKeyDown) return;
    switch(k.wVirtualKeyCode){
        case VK_ESCAPE: g_show_start=false; break;
        case VK_F1: create_window(18,4,44,14,L"File Manager",APP_FILES); break;
        case VK_F2: create_window(24,6,48,16,L"Text Editor",APP_EDITOR); break;
        case VK_F3: create_window(30,8,40,14,L"Calculator",APP_CALC); break;
        case '1': if(g_show_start) { open_app_from_icon(APP_FILES);  g_show_start=false; } break;
        case '2': if(g_show_start) { open_app_from_icon(APP_EDITOR); g_show_start=false; } break;
        case '3': if(g_show_start) { open_app_from_icon(APP_CALC);   g_show_start=false; } break;
        case VK_LWIN: case VK_RWIN: g_show_start=!g_show_start; break;
        default: break;
    }
}

static void handle_mouse(MOUSE_EVENT_RECORD m){
    int mx=m.dwMousePosition.X, my=m.dwMousePosition.Y; bool down=(m.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED)!=0;

    if(m.dwEventFlags==0){
        if(my==g_sh-1 && mx>=2 && mx<=8 && down){ g_show_start=!g_show_start; return; }

        for(int zi=g_win_count-1; zi>=0; zi--){
            Window* w=&g_windows[g_zorder[zi]]; Rect r=w->r;
            if(point_in(r,mx,my)){
                focus_window(zi);
                if(my==r.y && down){ w->dragging=true; w->drag_dx=mx-r.x; w->drag_dy=my-r.y; }
                if(my==r.y && mx>=r.x+r.w-4 && down){ close_window(w->id); }
                return;
            }
        }

        if(down){
            for(int i=0;i<g_icon_count;i++){
                Icon* ic=&g_icons[i]; Rect ir={ic->x, ic->y, (short)max(8,(int)wcslen(ic->label)), 2};
                if(point_in(ir,mx,my)) { open_app_from_icon(ic->app); return; }
            }
        }
    }

    if(m.dwEventFlags==MOUSE_MOVED){
        for(int i=0;i<MAX_WIN;i++) if(g_windows[i].used && g_windows[i].dragging){
            int nx = mx - g_windows[i].drag_dx; int ny = my - g_windows[i].drag_dy;
            if(nx<0) nx=0; if(ny<0) ny=0; if(nx+g_windows[i].r.w>g_sw) nx=g_sw-g_windows[i].r.w; if(ny+g_windows[i].r.h>g_sh-TASKBAR_H) ny=g_sh-TASKBAR_H-g_windows[i].r.h;
            g_windows[i].r.x=nx; g_windows[i].r.y=ny;
        }
    }

    if(!down){ for(int i=0;i<MAX_WIN;i++) g_windows[i].dragging=false; }
}

// ─────────────────────────────────────────────────────────────────────────────
// Init / main loop
// ─────────────────────────────────────────────────────────────────────────────

static void setup_console(){
    g_out = GetStdHandle(STD_OUTPUT_HANDLE);
    g_in  = GetStdHandle(STD_INPUT_HANDLE);

    SMALL_RECT r={0,0, (SHORT)(g_sw-1), (SHORT)(g_sh-1)};
    SetConsoleWindowInfo(g_out, TRUE, &r);
    SetConsoleScreenBufferSize(g_out, (COORD){(SHORT)g_sw,(SHORT)g_sh});
    SetConsoleWindowInfo(g_out, TRUE, &r);

    CONSOLE_CURSOR_INFO ci={1,FALSE}; SetConsoleCursorInfo(g_out,&ci);

    DWORD mode=0; GetConsoleMode(g_in,&mode);
    mode |= ENABLE_EXTENDED_FLAGS;
    mode &= ~(ENABLE_QUICK_EDIT_MODE);
    mode |= (ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);
    SetConsoleMode(g_in, mode);

    g_buf_front = (CHAR_INFO*)malloc(sizeof(CHAR_INFO)*g_sw*g_sh);
    g_buf_back  = (CHAR_INFO*)malloc(sizeof(CHAR_INFO)*g_sw*g_sh);

    clear_buf(g_buf_front,(WORD)(COL_BG|COL_FG),L' ');
    clear_buf(g_buf_back,(WORD)(COL_BG|COL_FG),L' ');
}

static void remove_closed_windows(){
    for(int i=0;i<MAX_WIN;i++) if(g_windows[i].used){
        if(!g_windows[i].closing){ g_windows[i].open_t = fminf(1.0f, g_windows[i].open_t + 0.10f); }
        else { g_windows[i].open_t = fmaxf(0.0f, g_windows[i].open_t - 0.12f); if(g_windows[i].open_t<=0.0f){ g_windows[i].used=false; }}
    }
    int nz=0; for(int i=0;i<g_win_count;i++){ int id=g_zorder[i]; if(g_windows[id].used) g_zorder[nz++]=id; }
    g_win_count=nz;
}

void start_GROS(void) {
    SetConsoleTitleW(L"RVM OS — Console GUI v3");
    setup_console();

    // --- fullscreen + фикс размера буфера ---
    HWND hwnd = GetConsoleWindow();

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME);
    SetWindowLong(hwnd, GWL_STYLE, style);

    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        SWP_FRAMECHANGED | SWP_SHOWWINDOW);

    // Размер консоли под твой виртуальный экран
    COORD bufSize = { SCREEN_W, SCREEN_H };
    SetConsoleScreenBufferSize(g_out, bufSize);

    SMALL_RECT winSize = { 0, 0, SCREEN_W-1, SCREEN_H-1 };
    SetConsoleWindowInfo(g_out, TRUE, &winSize);

    // --- запуск GUI ---
    init_icons();
    create_window(20,6,48,16,L"Welcome",APP_EDITOR);

    g_last_tick = GetTickCount();
    bool running = true;
    while (running) {
        DWORD now = GetTickCount(); 
        DWORD dt = now - g_last_tick; 
        if (dt < 1000/FPS) Sleep(1); 
        g_last_tick = now;

        DWORD ne = 0; 
        GetNumberOfConsoleInputEvents(g_in,&ne);
        while (ne--) { 
            INPUT_RECORD ev; DWORD rd; 
            ReadConsoleInputW(g_in,&ev,1,&rd);
            switch (ev.EventType) {
                case KEY_EVENT: handle_keyboard(ev.Event.KeyEvent); break;
                case MOUSE_EVENT: handle_mouse(ev.Event.MouseEvent); break;
                case WINDOW_BUFFER_SIZE_EVENT: /* ignore */ break;
            }
        }

        clear_buf(g_buf_back,(WORD)(COL_BG|COL_FG),L' ');
        draw_desktop(g_buf_back);
        for (int i = 0; i < g_win_count; i++) 
            draw_window(g_buf_back,&g_windows[g_zorder[i]]);
        draw_taskbar(g_buf_back);
        draw_start_menu(g_buf_back);

        swap_buffers(); 
        present();
        remove_closed_windows();

        SHORT ks  = GetAsyncKeyState(VK_MENU) & 0x8000; 
        SHORT ks2 = GetAsyncKeyState(VK_F4) & 0x8000;
        if (ks && ks2) running = false;
    }

    free(g_buf_front); 
    free(g_buf_back);
}

