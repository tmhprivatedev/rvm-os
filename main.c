#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <stdint.h>
#include <conio.h>

#define OSINDEX 0xF // kernel

#define MB 1024 * 1024
#define MEMSIZE MB
#define REGSIZE 12

#define REG1 0001
#define REG2 0010
#define REG3 0011
#define REG4 0100
#define REG5 0101
#define REG6 0110
#define REG7 0111
#define REG8 1000
#define REG01 1001
#define REG02 1010
#define REG03 1011	
#define REG04 1100

#define STRING_MEMSIZE 1024
#define STRING_MEMSTART (MEMSIZE - STRING_MEMSIZE)

void printColored(const char *text, int color) {
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	printf("%s", text);
	SetConsoleTextAttribute(hConsole, 7); // return to white color
	#define clr system("cls")
#else
	const char* colors[] = {"\033[31m", "\033[32m", "\033[33m"};
	printf("%s%s\033[0m", colors[color], text);
	#define clr system("clear")
#endif
}

typedef struct {
	uint8_t memory[MEMSIZE];
	bool running;
	int used_index;
	int string_mem_ptr;
} VM;

typedef struct {
	int index;
	int value;
} REG;

typedef struct {
	bool running;
	REG regs[REGSIZE];
	uint8_t output_mode;
	uint8_t ext_index;
	int pc;
	int instruction_counter;
	
} PROCESSOR;

void init(VM* vm) {
	vm->running = false;
	memset(vm->memory, 0, sizeof(vm->memory));
	vm->string_mem_ptr = STRING_MEMSTART;
}

void allocateMemory(VM* vm, int size) {

	int amount = 0;

	for (int i = 0; i < size; i++) {
		vm->memory[i] = 1;
		vm->used_index++; // WILL BE DELETED
	}

	for (int i = 0; i < MB; i++) {
		if (vm->memory[i] == 1) {
			amount++;
		}
	}

	printf("Total allocated memory amount: %d bytes\n", amount);
	printf("Used: %d\n", vm->used_index);
	
}

void allocateSpecialMemory(VM* vm, int index) {
	vm->memory[index] = 1;
}

void freeSpecialMemory(VM* vm, int index) {
	vm->memory[index] = 0;
}

void freeAllMemory(VM* vm) {
	// WILL CLEAR ALL USED VIRTUAL MEMORY

	for (int i = 0; i < MB; i++) {
		if (vm->memory[i] != 0) {
			vm->memory[i] = 0;
		}
	}
	printf("Cleared all virtual memory\n");
}

void putChar (VM* vm, char ch, int index) {
	vm->memory[index] = ch;
}

void putString(VM* vm, int st, int nd, char* text) {
	size_t lent = strlen(text);
	int length = (int)lent;
	for (int i = st; i < nd; i++) {
			for (int j = 0; j < length; j++) {
				vm->memory[j] = text[j];
			}
		}
}

void putInstr(VM* vm, uint16_t instr, int index) {
	
	for (int i = 0; i < 12; i++) {
		int bit = (instr >> (11 - i)) & 1;
		vm->memory[index + i] = bit;
	}

}

void printString(VM* vm, int start_addr) {
	int addr = start_addr;
	while (vm->memory[addr] != 0) {
		printf("%c", vm->memory[addr]);
		addr++;
	}
	printf("\n");
}

void printMemory(VM* vm, int st, int nd) {
	for (int i = st; i < nd; i++) {
		char cmd[1024];
		sprintf(cmd, "%d ", vm->memory[i]);
		if (vm->memory[i] != 0) {
			printColored(cmd, 12);
		} else {
			printColored(cmd, 2);
		}
	}
	printf("\n");
}

float calcForUsedMemory(VM* vm) {
	float result = 0;
	float used = 0;
	float unused = 0;

	for (int i = 0; i < MB; i++) {
		if (vm->memory[i] == 1) {
			used++;
		} else if (vm->memory[i] == 0) {
			unused++;
		}
	}

	result = (used / unused) * 100;

	return result;
}

void printSpecialInfo (VM* vm) {
	printf("Memory Amount: %d\n", MEMSIZE);
	printf("Total memory used: %f%%\n", calcForUsedMemory(vm));
}

void fillRegs(PROCESSOR* processor) {
	processor->regs[0].index = REG1;
	processor->regs[1].index = REG2;
	processor->regs[2].index = REG3;
	processor->regs[3].index = REG4;
	processor->regs[4].index = REG5;
	processor->regs[5].index = REG6;
	processor->regs[6].index = REG7;
	processor->regs[7].index = REG8;
	processor->regs[8].index = REG01;
	processor->regs[9].index = REG02;
	processor->regs[10].index = REG03;
	processor->regs[11].index = REG04;

	for (int i = 0; i < REGSIZE; i++) {
		processor->regs[i].value = 0;
	}
}

void processorInit (PROCESSOR* processor) {
	processor->running = false;
	fillRegs(processor);
	processor->output_mode = 0; // 0 decimals; 1 symbols;
	processor->ext_index = 0x0;
	processor->instruction_counter = 0;
}

int processorParse(VM* vm, int index) {
	uint16_t instr = 0;
	for (int i = 0; i < 12; i++) {
		instr = (instr << 1) | vm->memory[index + i];
	}
	return instr;	
}

uint32_t get_reg_addr (uint8_t reg_num) {
	return 0x1000 + (reg_num * 4);
}

extern int externalLoad();

void exec(VM* vm, PROCESSOR* cpu, uint16_t instr) {
	uint8_t first4 = (instr >> 8) & 0xF;
	uint8_t mid4   = (instr >> 4) & 0xF;
	uint8_t last4  = instr & 0xF;
	uint32_t reg_addr = 0;
	uint8_t key = 0;
	char string[256];	
	int int_string = 0;

	if (instr == 0x770) {
		vm->running = false;
		return;
	}

	if (instr == 0xF77) {
		vm->running = true;
		return;
	}

	switch (first4) {

		case 0xC: // PUT 1100
			cpu->regs[mid4].value = cpu->regs[last4].value;
			break;
		case 0xD: // JNZ 1101
			if (cpu->regs[mid4].value != 0) {
				cpu->pc = cpu->regs[last4].value;
			}
			break;
		case 0xE: // NCMPD 1110
			if (cpu->regs[mid4].value != last4) {
				vm->running = false;
			}
			break;
		case 0x7: // DELAY 0111

			switch (mid4) {
				case 0x2:
					reg_addr = get_reg_addr(last4);
					cpu->regs[11].value = reg_addr;
					break;
				case 0x4: // JMP
					cpu->pc = cpu->regs[last4].value;
					break;
				case 0x3:
					if (cpu->regs[last4].value != 0) {
						cpu->regs[mid4].value /= cpu->regs[last4].value;
					}
					break;
				case 0x6: // EXT
					if (last4 == 0xF) {
						externalLoad();
					}
						break;
				case 0xD: // INP
					key = getch();
					cpu->regs[REG04].value = key;
					break;
				case 0x1: // CLEARALL 0001
					clr;	
					memset(vm->memory, 0, MEMSIZE);
					for (int i = 0; i < REGSIZE; i++) {
						cpu->regs[i].value = 0;
					}
					vm->running = false;
					break;
				case 0x5:
					cpu->regs[last4].value = 0;
					break;
				default:
					break;
			}

			break;
			
		case 0x9: // PUTD 1001
			cpu->regs[mid4].value = last4;
			break;
		case 0x6: // SUB 0110
			cpu->regs[mid4].value -= cpu->regs[last4].value;
			break;
		case 0x1: // ADD 0001
			cpu->regs[mid4].value += cpu->regs[last4].value;
			break;
		case 0xB: // CHMD 1011
			if (cpu->output_mode == 0) {
				cpu->output_mode = 1;
			} else {
				cpu->output_mode = 0;
			}

			break;
		case 0xA: // MULT 1010
			cpu->regs[mid4].value *= cpu->regs[last4].value;
			break;
		case 0x8: // GET 1000
			if (cpu->output_mode == 0) {
				printf("%d", cpu->regs[mid4].value);
			} else {
				printf("%c", cpu->regs[mid4].value);
			}
			break;
		case 0x4: // NCMP
			if (cpu->regs[mid4].value != cpu->regs[last4].value) {
				vm->running = false;
			}
			break;
		case 0x2: // RRN
			if (mid4 == 0x9 ||
				mid4 == 0xA ||
				mid4 == 0xB ||
				mid4 == 0xC) {
					cpu->regs[mid4].value = last4;
					break;
				}
			break;
		
		case 0x3: // JZ
			if (cpu->regs[mid4].value == 0) {
				cpu->pc = cpu->regs[last4].value;
			}
			break;

		default:
			printf("UNKNOWN: 0x%X\n", first4);
	}
}

void run(VM* vm, PROCESSOR* cpu, int start_addr) {
	cpu->pc = start_addr;
	vm->running = true;

	while (vm->running && cpu->pc < MEMSIZE - 12) {
		uint16_t instr = processorParse(vm, cpu->pc);
		int old_pc = cpu->pc;
        
		exec(vm, cpu, instr);
		if (cpu->pc == old_pc) {
			cpu->pc += 12;
		}
	}
}


void loadProgram(VM* vm, const char* filename, int start_addr) {
	FILE* file = fopen(filename, "r");
	if (!file) {
		printf("ERROR: Cannot open %s\n", filename);
		return;
	}
	char line[20];
	int addr = start_addr;
	int count = 0;
	
	while (fgets(line, sizeof(line), file)) {
		if (line[0] == '\n' || line[0] == '\r' || line[0] == ';') {
			continue;
		}


		line[strcspn(line, "\n\r")] = 0;
		if (strlen(line) != 12) {
			continue;
		}


		uint16_t instr = 0;
		for (int i = 0; i < 12; i++) {
			if (line[i] == '1') {
				instr = (instr << 1) | 1;
			} else if (line[i] == '0') {
				instr = (instr << 1) | 0;
			} else {
				instr = 0;
				break;
			}
		}
		putInstr(vm, instr, addr);
		addr += 12;
		count++;
		
		if (addr >= MEMSIZE) {
			printf("MEMORY FULL");
			break;
		}
	}

	fclose(file);
}

int main () {

	VM vm;
	PROCESSOR processor;

	init(&vm);
	processorInit(&processor);

	loadProgram(&vm, "rvm\\KERNEL.rvm", 256);
	loadProgram(&vm, "rvm\\BOOT.rvm", 0);

	run(&vm, &processor, 0);
			
	return 0;
}
