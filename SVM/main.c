#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "parseinstrs.h"
#define STACK_SIZE 20

int8_t stack[STACK_SIZE] = {0};
int8_t regs[] = {
	[R1] = 0,
	[R2] = 0,
	[R3] = 0,
	[R4] = 0,
	[RBP] = 0,
	[RSP] = 0,
	[RIP] = 0
};

static inline const char *reg_enum_to_str(enum Reg type)
{
	switch (type)
	{
		case R1: return "R1";
		case R2: return "R2";
		case R3: return "R3";
		case R4: return "R4";
		case RBP: return "RBP";
		case RSP: return "RSP";
		case RIP: return "RIP";
	}
}

static void run_instr(struct Instr *instr)
{
	switch (instr->type)
	{
		case PRINTREG:
			printf("REG %s VALUE: %d\n", reg_enum_to_str(instr->arg1.reg),
			  regs[instr->arg1.reg]);
			break;
		case INC:	// i will not be handling signed integer overflows lol (undefined behavior)
			regs[instr->arg1.reg] += 1; break;
		case DEC:
			regs[instr->arg1.reg] -= 1; break;
		case PUSH:
		case POP:
		case MOV:
		case LDA:
		case ADD:
		case SUB:
		case MUL:
	}
}

int main(void)
{
	char src[] =
	"MOV R1 R2;"
	"MOV R3 R4;"
	"PUSH R3;"
	"MOV R3 R4;"
	"MOV R3 123;";

	int instrs_len;
	struct Instr *parsed_instrs = parse_src(src, &instrs_len);

	printf("___DEBUG INSTRUCTIONS:\n");
	for (int i  = 0; i < instrs_len; i++)
		___debug_instr(parsed_instrs + i);
	return 0;
}

