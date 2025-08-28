#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

#ifdef _WIN32
#define clr system("cls")
#else
#define clr system("clear")
#endif

#define COLOR_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COLOR_BLACK 0
#define COLOR_RED FOREGROUND_RED
#define COLOR_GREEN FOREGROUND_GREEN
#define COLOR_BLUE FOREGROUND_BLUE
#define COLOR_YELLOW (FOREGROUND_RED | FOREGROUND_GREEN)
#define COLOR_CYAN (FOREGROUND_GREEN | FOREGROUND_BLUE)
#define COLOR_MAGENTA (FOREGROUND_RED | FOREGROUND_BLUE)

#define BG_BLACK 0
#define BG_WHITE (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)
#define BG_RED BACKGROUND_RED
#define BG_GREEN BACKGROUND_GREEN
#define BG_BLUE BACKGROUND_BLUE
#define BG_YELLOW (BACKGROUND_RED | BACKGROUND_GREEN)
#define MAX_WINDOWS 5

// ==================== СТЕЛС-СИСТЕМА ====================

void stealth_console() {
    // 1. Меняем заголовок окна
    SetConsoleTitleA("RVN OS Kernel Mode");
    
    // 2. Прячем курсор навсегда
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
    
    // 3. Убираем прокрутку
    HWND consoleWindow = GetConsoleWindow();
    ShowScrollBar(consoleWindow, SB_BOTH, FALSE);
    
    // 4. Запрещаем изменение размера
    SetWindowLong(consoleWindow, GWL_STYLE, 
                 GetWindowLong(consoleWindow, GWL_STYLE) & ~WS_SIZEBOX);
    
    // 5. Максимизируем на весь экран
    ShowWindow(consoleWindow, SW_MAXIMIZE);
    
    // 6. Фиксируем размер буфера
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    csbi.dwSize.X = 80;
    csbi.dwSize.Y = 25;
    SetConsoleScreenBufferSize(hConsole, csbi.dwSize);
}

void os_input_handler() {
    // Перехватываем системные сообщения
    HWND consoleWindow = GetConsoleWindow();
    
    // Убираем стандартное меню
    HMENU hMenu = GetSystemMenu(consoleWindow, FALSE);
    DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
    DeleteMenu(hMenu, SC_SIZE, MF_BYCOMMAND);
}

int os_getch() {
    // Собственный обработчик ввода без эха
    HANDLE hConsole = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD inputRecord;
    DWORD eventsRead;
    
    while (ReadConsoleInput(hConsole, &inputRecord, 1, &eventsRead)) {
        if (inputRecord.EventType == KEY_EVENT && 
            inputRecord.Event.KeyEvent.bKeyDown) {
            return inputRecord.Event.KeyEvent.uChar.AsciiChar;
        }
    }
    return 0;
}

void draw_fake_taskbar() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 24};
    
    // Рисуем панель задач
    SetConsoleTextAttribute(hConsole, BG_BLUE | COLOR_WHITE);
    for (int i = 0; i < 80; i++) {
        pos.X = i;
        SetConsoleCursorPosition(hConsole, pos);
        printf(" ");
    }
    
    // Время и дата
    pos.X = 60;
    SetConsoleCursorPosition(hConsole, pos);
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    printf(" %02d:%02d %02d/%02d/%d ", 
           tm.tm_hour, tm.tm_min, 
           tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
}

void fade_in() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    // Плавное появление
    for (int i = 0; i <= 100; i += 5) {
        SetConsoleTextAttribute(hConsole, 
            FOREGROUND_INTENSITY | (i * 15 / 100));
        Sleep(30);
    }
    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
}

void screen_transition() {
    system("cls");
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Эффект перехода
    for (int y = 0; y < 25; y++) {
        for (int x = 0; x < 80; x++) {
            COORD pos = {x, y};
            SetConsoleCursorPosition(hConsole, pos);
            printf("%c", 178 + rand() % 4);
        }
        Sleep(50);
    }
    system("cls");
}

void system_monitor() {
    while (1) {
        system("cls");
        printf("RVN OS System Monitor\n");
        printf("=====================\n");
        printf("CPU Usage:    %d%%\n", 10 + rand() % 50);
        printf("Memory:       %dMB/%dMB\n", 200 + rand() % 300, 512);
        printf("Processes:    %d\n", 15 + rand() % 10);
        printf("Uptime:       %02d:%02d:%02d\n", 
               rand() % 24, rand() % 60, rand() % 60);
        
        printf("\nPress ESC to return...");
        
        if (os_getch() == 27) break;
        Sleep(1000);
    }
}

// ==================== ГРАФИЧЕСКАЯ СИСТЕМА ====================

typedef struct {
    int x, y;
    int width, height;
    char title[50];
    void (*draw_content)();
} Window;

Window windows[MAX_WINDOWS];
int window_count = 0;

void fast_draw_rect(int x, int y, int width, int height, WORD color, char fill_char) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos;
    
    SetConsoleTextAttribute(hConsole, color);
    
    for (int i = 0; i < height; i++) {
        pos.X = x;
        pos.Y = y + i;
        SetConsoleCursorPosition(hConsole, pos);
        
        for (int j = 0; j < width; j++) {
            printf("%c", fill_char);
        }
    }
}

void fast_draw_window(int x, int y, int width, int height, const char* title) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Рамка окна
    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
    
    COORD pos = {x, y};
    SetConsoleCursorPosition(hConsole, pos);
    printf("+");
    for (int i = 0; i < width - 2; i++) printf("-");
    printf("+");
    
    // Заголовок
    pos.Y = y + 1;
    pos.X = x;
    SetConsoleCursorPosition(hConsole, pos);
    printf("|");
    SetConsoleTextAttribute(hConsole, BACKGROUND_BLUE | COLOR_WHITE);
    printf(" %-*s ", width - 4, title);
    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
    printf("|");
    
    // Боковые стороны
    for (int i = 2; i < height - 1; i++) {
        pos.Y = y + i;
        pos.X = x;
        SetConsoleCursorPosition(hConsole, pos);
        printf("|");
        for (int j = 0; j < width - 2; j++) printf(" ");
        printf("|");
    }
    
    // Нижняя рамка
    pos.Y = y + height - 1;
    pos.X = x;
    SetConsoleCursorPosition(hConsole, pos);
    printf("+");
    for (int i = 0; i < width - 2; i++) printf("-");
    printf("+");
    
    SetConsoleTextAttribute(hConsole, COLOR_WHITE);
}

void add_window(int x, int y, int w, int h, const char* title, void (*draw_func)()) {
    if (window_count < MAX_WINDOWS) {
        windows[window_count].x = x;
        windows[window_count].y = y;
        windows[window_count].width = w;
        windows[window_count].height = h;
        strcpy(windows[window_count].title, title);
        windows[window_count].draw_content = draw_func;
        window_count++;
    }
}

void draw_desktop() {
    system("cls");
    fast_draw_rect(0, 0, 80, 25, BACKGROUND_GREEN, '.');
}

void redraw_all() {
    draw_desktop();
    
    for (int i = 0; i < window_count; i++) {
        fast_draw_window(windows[i].x, windows[i].y, 
                        windows[i].width, windows[i].height, 
                        windows[i].title);
    }
    
    draw_fake_taskbar();
}

// ==================== ОСНОВНЫЕ ФУНКЦИИ ====================

void show_logo_tech() {
    system("cls");
    system("chcp 65001>nul");
    printf("\n");
    printf("┌────────────────────────────────────┐\n");
    printf("│  ██████╗ ██╗   ██╗███╗   ███╗      │\n");
    printf("│  ██╔══██╗██║   ██║████╗ ████║      │\n");
    printf("│  ██████╔╝██║   ██║██╔████╔██║      │\n");
    printf("│  ██╔══██╗██║   ██║██║╚██╔╝██║      │\n");
    printf("│  ██║  ██║╚██████╔╝██║ ╚═╝ ██║      │\n");
    printf("│  ╚═╝  ╚═╝ ╚═════╝ ╚═╝     ╚═╝      │\n");
    printf("│  ────────────────────────────────  │\n");
    printf("│        O P E R A T I N G           │\n");
    printf("│          S Y S T E M               │\n");
    printf("└────────────────────────────────────┘\n");
}

int quick_menu() {
    system("cls");
    printf("+--------------------------+\n");
    printf("|       RVM OS MENU        |\n");
    printf("+--------------------------+\n");
    printf("| [1] System Monitor       |\n");
    printf("| [2] File Manager         |\n");
    printf("| [3] Text Editor          |\n");
    printf("| [4] Calculator           |\n");
    printf("| [5] Return to Boot Menu  |\n");
    printf("| [0] Shutdown             |\n");
    printf("+--------------------------+\n");
    printf("Choice: ");
    
    return os_getch();
}

int conOS() {
    system("mode con: cols=80 lines=25");
    
    while (1) {
        int choice = quick_menu();
        
        switch (choice) {
            case '1':
                system_monitor();
                break;
            case '5':
                return 1;  // Return to main menu
            case '0':
                return 0;  // Shutdown
            default:
                printf("\nFeature in development!\n");
                Sleep(1000);
                break;
        }
    }
}

int externalLoad() {
    stealth_console();
    os_input_handler();
    
    while (1) {
        fade_in();
        show_logo_tech();
        
        printf("\nRVN OS Boot Manager\n");
        printf("===================\n");
        printf("[1] Start RVM OS\n");
        printf("[2] System Settings\n");
        printf("[3] Recovery Mode\n");
        printf("[0] Power Off\n");
        printf("Choice: ");
        
        int key = os_getch();
        
        switch (key) {
            case '1':
                screen_transition();
                int result = conOS();
                screen_transition();
                if (result == 0) return 0;
                break;
            case '0':
                printf("\nShutting down...\n");
                Sleep(2000);
                return 0;
            default:
                break;
        }
        
        draw_fake_taskbar();
    }
}

int main() {
    // Запускаем нашу СТЕЛС-ОС
    return externalLoad();
}