#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INSTRUCTION_SIZE 12
#define MAX_LINE_LENGTH 100

// Таблица инструкций
typedef struct {
    char *name;
    char *code;
} Instruction;

Instruction instructions[] = {
    {"STOP", "0000"},
    {"START", "1111"},
    {"PUT", "1100"},
    {"PUTD", "1001"},
    {"SUB", "0110"},
    {"CLEAR", "1101"},
    {"CLEARALL", "1110"},
    {"DELAY", "0111"},
    {"CHMD", "1011"},
    {"MULT", "1010"},
    {"GET", "1000"},
    {"ADD", "0001"},
    {"RRN", "0010"},
    {"CMP", "0101"},
    {"NCMP", "0100"},
    {"DIV", "0011"},
    // EXTERNAL INSTRUCTIONS
    {"JMP", "0100"},
    {"GETADDR", "0010"},
    {"GOTO", "1010"},
    {"INP", "1101"},
    {"TINP", "1100"},
    {"EXT", "0110"}
};

int num_instructions = sizeof(instructions) / sizeof(instructions[0]);

void int_to_binary(int num, int bits, char *output) {
    for (int i = bits - 1; i >= 0; i--) {
        output[bits - 1 - i] = (num & (1 << i)) ? '1' : '0';
    }
    output[bits] = '\0';
}

char* find_instruction_code(char *name) {
    for (int i = 0; i < num_instructions; i++) {
        if (strcmp(instructions[i].name, name) == 0) {
            return instructions[i].code;
        }
    }
    return NULL;
}

int assemble_line(char *line, char *binary_output) {
    char instruction[20], arg1[20], arg2[20];
    int args = sscanf(line, "%s %[^,], %s", instruction, arg1, arg2);
    if (args < 2) args = sscanf(line, "%s %s", instruction, arg1);
    
    char *instr_code = find_instruction_code(instruction);
    if (!instr_code) return -1;
    
    char binary[INSTRUCTION_SIZE + 1] = {0};
    
    // Проверяем是否是 внешняя команда
    int is_external = 0;
    char *external_instructions[] = {"JMP", "GETADDR", "GOTO", "INP", "TINP", "EXT", NULL};
    for (int i = 0; external_instructions[i] != NULL; i++) {
        if (strcmp(instruction, external_instructions[i]) == 0) {
            is_external = 1;
            break;
        }
    }
    
    if (is_external) {
        // Внешние команды: DELAY + код_команды + аргументы
        strcpy(binary, "0111"); // Всегда начинаем с DELAY
        strcat(binary, instr_code);
    } else {
        // Обычные команды
        strcpy(binary, instr_code);
    }
    
    // Обработка аргументов
    if (args > 1) {
        if (arg1[0] == 'R') {
            int reg_num = atoi(arg1 + 1);
            char reg_binary[5];
            int_to_binary(reg_num, 4, reg_binary);
            strcat(binary, reg_binary);
        } else {
            int value = atoi(arg1);
            char value_binary[5];
            int_to_binary(value, 4, value_binary);
            strcat(binary, value_binary);
        }
        
        if (args > 2) {
            int value = atoi(arg2);
            char value_binary[5];
            int_to_binary(value, 4, value_binary);
            strcat(binary, value_binary);
        }
    }
    
    // Дополняем DELAY до 12 бит
    int current_len = strlen(binary);
    if (current_len < INSTRUCTION_SIZE) {
        int needed = INSTRUCTION_SIZE - current_len;
        for (int i = 0; i < needed/4; i++) {
            strcat(binary, "0111");
        }
    }
    
    strcpy(binary_output, binary);
    return 0;
}
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input.rasm output.rvm\n", argv[0]);
        return 1;
    }
    
    FILE *input = fopen(argv[1], "r");
    FILE *output = fopen(argv[2], "w");
    
    if (!input || !output) return 1;
    
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, input)) {
        if (line[0] == ';' || line[0] == '\n') continue;
        line[strcspn(line, "\n\r")] = 0;
        
        char binary[INSTRUCTION_SIZE + 1];
        if (assemble_line(line, binary) == 0) {
            fprintf(output, "%s\n", binary);
        }
    }
    
    fclose(input);
    fclose(output);
    return 0;
}