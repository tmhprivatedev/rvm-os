#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <conio.h>
#include <windows.h>

#ifdef _WIN32
#define clr system("cls")
#else
#define clr system("clear")
#endif

typedef struct {
    int value;
    int index;
} Position;

typedef struct {
    char name[128];  // исправлено: было char name;
    int position;
    char folder_name[128];
    char value[10000];
} File;

typedef struct {
    char name[128];
    bool is_files_exist;
    File files[12];
} Folder;

typedef struct {
    File files[1024];
    Folder folders[1024];
    bool is_inited;
    Position positions[1024];
} FS;

void init(FS* fs) {
    fs->is_inited = true;
    for (int i = 0; i < 1024; i++) {
        fs->positions[i].value = 0;
        fs->positions[i].index = i;
    }
    printf("File system initialized!\n");
}

int isInited(FS* fs) {
    return fs->is_inited ? 0 : 1;
}

int getFreePosition(FS* fs) {
    for (int i = 0; i < 1024; i++) {
        if (fs->positions[i].value == 0) {
            return i;
        }
    }
    return -1;
}

void createFile(FS* fs, char filename[128]) {
    if (isInited(fs) == 0) {
        int pos = getFreePosition(fs);
        if (pos == -1) {
            printf("No free space!\n");
            return;
        }
        fs->files[pos].position = pos;
        strcpy(fs->files[pos].name, filename);
        fs->positions[pos].value = 1;
        printf("Successfully created file: %s\n", filename);
    } else {
        printf("File system not initialized! Use 'initFS' first.\n");
    }
}

void debugShell(FS* fs) {
    printf("=== DEBUG ===\n");
    printf("is_inited: %d\n", fs->is_inited);
    printf("Press any key to continue...\n");
    getch();
}

void open_shell() {
    clr;
    printf("=== FSROS SHELL ===\n");
    printf("Type 'help' for commands\n\n");
    
    FS fs;
    // Явно инициализируем структуру
    memset(&fs, 0, sizeof(FS));
    fs.is_inited = false;
    
    char command[256];
    
    while (1) {
        printf("> ");
        
        // Простой ввод без сложных проверок
        if (scanf("%255s", command) != 1) {
            break;
        }
        
        // Очистка буфера
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
        
        printf("Executing: %s\n", command);  // Отладочное сообщение
        
        if (strcmp(command, "help") == 0) {
            printf("Available commands:\n");
            printf("initFS - Initialize file system\n");
            printf("create - Create file or folder\n");
            printf("debug - Debug info\n");
            printf("quit - Exit shell\n");
        }
        else if (strcmp(command, "initFS") == 0) {
            init(&fs);
        }
        else if (strcmp(command, "create") == 0) {
            printf("Create file or folder? ");
            char type[20];
            scanf("%19s", type);
            while ((c = getchar()) != '\n' && c != EOF);
            
            if (strcmp(type, "file") == 0) {
                printf("Filename: ");
                char filename[128];
                scanf("%127s", filename);
                while ((c = getchar()) != '\n' && c != EOF);
                createFile(&fs, filename);
            } else {
                printf("Creating folder...\n");
            }
        }
        else if (strcmp(command, "debug") == 0) {
            debugShell(&fs);
        }
        else if (strcmp(command, "quit") == 0) {
            printf("Exiting shell...\n");
            Sleep(1000);
            return;
        }
        else {
            printf("Unknown command: %s\n", command);
        }
        
        printf("\n");
    }
}

void boot() {
    while (1) {
        clr;
        printf("=========================\n");
        printf("|        FSROS          |\n");
        printf("=========================\n");
        printf("An OS with file system.\n\n");
        printf("1 - Open shell\n");
        printf("2 - About FSROS\n");
        printf("3 - Quit\n\n");
        printf("Select: ");
        
        char choice = getch();
        printf("%c\n", choice);
        
        switch (choice) {
            case '1':
                printf("Opening shell...\n");
                Sleep(1000);
                open_shell();
                break;
                
            case '2':
                printf("FSROS v1.0 - Simple File System OS\n");
                printf("Press any key to continue...\n");
                getch();
                break;
                
            case '3':
                printf("Goodbye!\n");
                Sleep(1000);
                exit(0);
                
            default:
                printf("Invalid choice! Press any key...\n");
                getch();
                break;
        }
    }
}

int main() {
    printf("Starting FSROS...\n");
    Sleep(1000);
    boot();
    return 0;
}