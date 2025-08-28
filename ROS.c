#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include "GROS.h"

#ifdef _WIN32
#define clr system("cls")
#else
#define clr system("clear")
#endif

#define WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define MAX_WINDOWS 5

#define CONFIG_FILE "rvm\\system.cfg"
#define ROS_MODE 0
#define GROS_MODE 1

typedef struct {
    int x, y;
    int width, height;
    char title[50];
    void (*draw_content)();
} Window;

Window windows[MAX_WINDOWS];
int window_count = 0;

// Быстрая отрисовка прямоугольника
void fast_draw_rect(int x, int y, int width, int height, WORD color, char fill_char) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
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

// Быстрое окно с ASCII рамкой
void fast_draw_window(int x, int y, int width, int height, const char* title) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    
    // Рамка окна
    SetConsoleTextAttribute(hConsole, WHITE);
    
    // Верхняя рамка
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
    SetConsoleTextAttribute(hConsole, BACKGROUND_BLUE | WHITE);
    printf(" %-*s ", width - 4, title);
    SetConsoleTextAttribute(hConsole, WHITE);
    printf("|");
    
    // Боковые стороны и содержимое
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
    
    SetConsoleTextAttribute(hConsole, WHITE);
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

void close_window(int index) {
    if (index >= 0 && index < window_count) {
        for (int i = index; i < window_count - 1; i++) {
            windows[i] = windows[i + 1];
        }
        window_count--;
    }
}

void draw_desktop() {
    system("cls");
    // Используем ASCII символ для фона
    fast_draw_rect(0, 0, 80, 25, FOREGROUND_GREEN, '.');
}

void redraw_all() {
    draw_desktop();
    
    // Рисуем все окна
    for (int i = 0; i < window_count; i++) {
        fast_draw_window(windows[i].x, windows[i].y, 
                        windows[i].width, windows[i].height, 
                        windows[i].title);
        
        if (windows[i].draw_content) {
            windows[i].draw_content();
        }
    }
    
    // Панель задач
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {0, 24};
    SetConsoleCursorPosition(hConsole, pos);
    SetConsoleTextAttribute(hConsole, BACKGROUND_BLUE | WHITE);
    printf(" RVM OS v0.0.1 [1]File [2]Edit [3]Calc [X]Close ");
    SetConsoleTextAttribute(hConsole, WHITE);
}

// Простые функции содержимого окон
void draw_file_manager_content() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {windows[0].x + 2, windows[0].y + 2};
    
    SetConsoleCursorPosition(hConsole, pos);
    printf("C:\\");
    pos.Y++;
    SetConsoleCursorPosition(hConsole, pos);
    printf("Windows\\");
    pos.Y++;
    SetConsoleCursorPosition(hConsole, pos);
    printf("Program Files\\");
}

void draw_text_editor_content() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {windows[0].x + 2, windows[0].y + 2};
    
    SetConsoleCursorPosition(hConsole, pos);
    printf("Text Editor");
    pos.Y += 2;
    SetConsoleCursorPosition(hConsole, pos);
    printf("+--------------------+");
    pos.Y++;
    SetConsoleCursorPosition(hConsole, pos);
    printf("|                    |");
}

void draw_calculator_content() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {windows[0].x + 2, windows[0].y + 2};
    
    SetConsoleCursorPosition(hConsole, pos);
    printf("Calculator");
    pos.Y++;
    SetConsoleCursorPosition(hConsole, pos);
    printf("7 8 9 +");
    pos.Y++;
    SetConsoleCursorPosition(hConsole, pos);
    printf("4 5 6 -");
}

void handle_input() {
    if (_kbhit()) {
        int key = _getch();
        
        switch (key) {
            case '1':
                add_window(5, 3, 40, 15, "File Manager", draw_file_manager_content);
                break;
            case '2':
                add_window(10, 2, 35, 12, "Text Editor", draw_text_editor_content);
                break;
            case '3':
                add_window(15, 4, 30, 10, "Calculator", draw_calculator_content);
                break;
            case 'x': case 'X':
                if (window_count > 0) close_window(window_count - 1);
                break;
        }
        redraw_all();
    }
}

int get_file_manager_key() {
    printf("\n");
    printf("+--------------------------+\n");
    printf("|       FILE MANAGER       |\n");
    printf("+--------------------------+\n");
    printf("| [1] Create Folder        |\n");
    printf("| [2] Delete Folder        |\n");
    printf("| [3] Create File          |\n");
    printf("| [4] Delete File          |\n");
    printf("| [0] Exit                 |\n");
    printf("+--------------------------+\n");
    printf("Choice: ");
    
    return _getch();
}

int quick_menu() {
    system("cls");
    printf("+--------------------------+\n");
    printf("|       RVM OS MENU        |\n");
    printf("+--------------------------+\n");
    printf("| [1] File Manager         |\n");
    printf("| [2] Text Editor          |\n");
    printf("| [3] Calculator           |\n");
    printf("| [4] Return               |\n");
    printf("| [0] Exit                 |\n");
    printf("+--------------------------+\n");
    printf("Choice: ");
    
    return _getch();
}


int current_gui_mode = ROS_MODE;

// Функция чтения конфигурации
int read_config() {
    FILE *file = fopen(CONFIG_FILE, "r");
    if (!file) {
        return ROS_MODE; // По умолчанию ROS
    }
    
    int mode;
    fscanf(file, "%d", &mode);
    fclose(file);
    
    return (mode == GROS_MODE) ? GROS_MODE : ROS_MODE;
}

// Функция записи конфигурации
void write_config(int mode) {
    FILE *file = fopen(CONFIG_FILE, "w");
    if (file) {
        fprintf(file, "%d", mode);
        fclose(file);
    }
}

// Функция установки GROS
void install_GROS() {
    system("cls");
    printf("Installing GROS Graphical System...\n");
    printf("===================================\n");
    
    for (int i = 0; i < 5; i++) {
        printf(".");
        Sleep(500);
    }
    
    write_config(GROS_MODE);
    current_gui_mode = GROS_MODE;
    
    start_GROS();

    printf("\n\nGROS installed successfully!\n");
    printf("System will boot with GROS on next startup.\n");
    Sleep(2000);
}

// Функция установки ROS
void install_ROS() {
    system("cls");
    printf("Installing ROS Graphical System...\n");
    printf("===================================\n");
    
    for (int i = 0; i < 5; i++) {
        printf(".");
        Sleep(500);
    }
    
    write_config(ROS_MODE);
    current_gui_mode = ROS_MODE;
    
    printf("\n\nROS installed successfully!\n");
    printf("System will boot with ROS on next startup.\n");
    Sleep(2000);
}

int conOS() {
    system("mode con: cols=80 lines=25");
    
    while (1) {
        int choice = quick_menu();
        
        switch (choice) {
            case '1':
                system("cls");
                printf("=== FILE MANAGER ===\n");
                printf("system/home/desktop/\n");
                int files_key = get_file_manager_key();
                break;
                
            case '2':
                system("cls");
                printf("=== TEXT EDITOR ===\n");
                printf("Press any key to return...\n");
                _getch();
                break;
                
            case '3':
                system("cls");
                printf("=== CALCULATOR ===\n");
                printf("Press any key to return...\n");
                _getch();
                break;
                
            case '4': 
                return 1; 
                
            case '0': 
                return 0; 
        }
    }
}

void set_giant_dos_font_registry() {
    system("powershell -command \""
           "$HKCU = 'HKCU:\\Console'; "
           "Set-ItemProperty $HKCU 'FaceName' 'Terminal'; "
           "Set-ItemProperty $HKCU 'FontSize' 0x00300018; "  // 48x24
           "Set-ItemProperty $HKCU 'FontFamily' 0; "
           "Set-ItemProperty $HKCU 'FontWeight' 400; "
           "\"");
    
    system("mode con: cols=16 lines=8");
    ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
}

void show_logo_tech() {
    clr;
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

void setup_console() {
    system("color 1f");
    set_giant_dos_font_registry();
    ShowWindow(GetConsoleWindow(), SW_MAXIMIZE);
    
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.dwSize = 1;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cursorInfo);
}

void reboot_application() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    
    ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOW);
    
    exit(0);
}

int installGraphicalOS () {
    clr;
    system("color 0f");
    printf("INSTALLING...\n");
    Sleep(1000);
    printf("INSTALLING GRAPHICAL INTERFACE...\n");
    Sleep(1000);
    for (int i = 0; i < 25; i++) {
        clr;
        printf("INSTALLING...\n");
        printf("INSTALLING GRAPHICAL INTERFACE...\n");
        printf("%d/25", i+1);
        Sleep(250);
    }
    printf("\n");
    printf("Applying...");
    Sleep(1000);
    install_GROS();
    return 1;
}

int externalLoad() {
    // Читаем конфигурацию при загрузке
    current_gui_mode = read_config();
    
    setup_console();
    show_logo_tech();
    
    while (1) {
        printf("\nRVN OS Boot Manager\n");
        printf("===================\n");
        printf("Current GUI: %s\n", (current_gui_mode == GROS_MODE) ? "GROS" : "ROS");
        printf("\nOptions:\n");
        printf("  [1] Start RVN OS (%s)\n", (current_gui_mode == GROS_MODE) ? "GROS" : "ROS");
        printf("  [2] Install GROS (Advanced GUI)\n");
        printf("  [3] Install ROS (Basic GUI)\n");
        printf("  [4] System Settings\n");
        printf("  [0] Power Off\n");
        printf("Choice: ");

        int key = _getch();
        printf("%c\n", key);

        switch (key) {
            case '1':
                printf("Booting with %s...\n", 
                      (current_gui_mode == GROS_MODE) ? "GROS" : "ROS");
                Sleep(1000);
                
                // Запускаем соответствующую GUI систему
                if (current_gui_mode == GROS_MODE) {
                    start_GROS(); // Ваша новая функция для GROS
                } else {
                    conOS(); // Старая ROS система
                }
                break;
                
            case '2':
                install_GROS();
                break;
                
            case '3':
                install_ROS();
                break;
                
            case '0':
                printf("Shutting down...\n");
                Sleep(1000);
                return 0;
                
            default:
                printf("Invalid choice!\n");
                Sleep(1000);
                break;
        }
        
        // Перерисовываем меню
        system("cls");
        show_logo_tech();
    }
}