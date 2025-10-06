#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "parseinstrs.h"
#define ERREXIT(...) fprintf(stderr, __VA_ARGS__), exit(1)
#define STACK_SIZE 20

int8_t stack[STACK_SIZE] = {0};
int8_t REGS[] = {
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
			  REGS[instr->arg1.reg]);
			break;
			
		case INC:	// i will not be handling signed integer overflows lol (undefined behavior)
			REGS[instr->arg1.reg] += 1;
			break;
			
		case DEC:
			REGS[instr->arg1.reg] -= 1;
			break;
			
		case PUSH:
			if ((int) REGS[RIP] == STACK_SIZE)
				ERREXIT("SVM Stack overflow\n");

			stack[REGS[RIP]] = instr->arg1_type == INT8 ?
			  instr->arg1.int8 :
			  REGS[instr->arg1.reg];
			REGS[RIP]++;
			break;
			
		case POP:
			if ((int) REGS[RIP] == 0)
				ERREXIT("SVM Stack underflow\n");

			REGS[RIP]--;
			REGS[instr->arg1.reg] = stack[REGS[RIP]];
			stack[REGS[RIP]] = 0;
			break;

		case MOV:
			 REGS[instr->arg1.reg] = instr->arg2_type == INT8 ?
			   instr->arg2.int8 :
			   REGS[instr->arg2.reg];
			 break;
			 
		case LDA:
			int stack_addr = (int8_t) (instr->arg2_type == INT8 ?
			  instr->arg2.int8 :
			  REGS[instr->arg2.reg]);
			if (stack_addr < 0 || stack_addr > 50)
				ERREXIT("SVM Segfault\n");
			
			REGS[instr->arg1.reg] = stack[stack_addr];
			break;

		case ADD:
			 REGS[instr->arg1.reg] += (int8_t) (instr->arg2_type == INT8 ?
			   instr->arg2.int8 :
			   REGS[instr->arg2.reg]);
			 break;			
		
		case SUB:
			 REGS[instr->arg1.reg] -= (int8_t) (instr->arg2_type == INT8 ?
			   instr->arg2.int8 :
			   REGS[instr->arg2.reg]);
			 break;			

		case MUL:
			 REGS[instr->arg1.reg] *= (int8_t) (instr->arg2_type == INT8 ?
			   instr->arg2.int8 :
			   REGS[instr->arg2.reg]);
			 break;	
	}
}

int main(void)
{
	char src[] =
	"PUSH 23;"
	"PUSH 126;"
	"POP R2;"
	"PRINTREG R2;"
	"LDA R1 0;"
	"PRINTREG R1;"
	"MOV R2 25;"
	"ADD R1 R2;"
	"MUL R1 -2;"
	"PRINTREG R1;";

	int instrs_len;
	struct Instr *parsed_instrs = parse_src(src, &instrs_len);

	printf("___DEBUG INSTRUCTIONS:\n");
	for (int i = 0; i < instrs_len; i++)
		___debug_instr(parsed_instrs + i);

	printf("RUNNING INSTRUCTIONS:\n");
	for (int i = 0; i < instrs_len; i++)
		run_instr(parsed_instrs + i);
	return 0;
}

