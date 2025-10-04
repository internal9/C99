// this is awful
/*
	TODO Here:
		- Finish parsing arguments
		- Array (possibly dynamic) to store parsed instructions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>

#define ERREXIT(...) fprintf(stderr, __VA_ARGS__), exit(0)

// gonna move these enum and struct definitions into a header file
// and the base datatype of enumerations are implementation-defined!!!
enum InstrType {
	NOP,	// special placeholder solely for the first "instruction" to serve as a pointer
	
	// One arg instructions
	INC,
	DEC,
	PUSH,
	POP,

	// Two arg instructions
	MOV,
	LDA,
	ADD,
	SUB,
	MUL,
};

enum Reg {
	R1,
	R2,
	R3,
	R4,
	RBP,
	RSP,
	RIP
};

enum ArgType {
	REG,
	INT8
};

struct INSTR_MATCH_INFO {
	char *name;
	int name_len;
	enum InstrType enum_type;
	bool uses_two_args;
};

struct REG_MATCH_INFO {
	char *name;
	int name_len;
	enum Reg enum_type;
};

struct Instr {
	enum InstrType type;
	enum ArgType arg1_type;
	enum ArgType arg2_type;
	union {
		enum Reg reg;
		int8_t int8;
	} arg1;
	union {
		enum Reg reg;
		int8_t int8;
	} arg2;
};

// 'name_len' in case i want to add instructions with identiifers of varying length
static const struct INSTR_MATCH_INFO INSTRS[] = {	// ragged array
	{.name = "INC", .name_len = 3, .enum_type = INC, .uses_two_args = false},
	{.name = "DEC", .name_len = 3, .enum_type = DEC, .uses_two_args = false},
	{.name = "PUSH", .name_len = 4, .enum_type = PUSH, .uses_two_args = false},
	{.name = "POP", .name_len = 3, .enum_type = POP, .uses_two_args = false},
	{.name = "MOV", .name_len = 3, .enum_type = MOV, .uses_two_args = true},
	{.name = "LDA", .name_len = 3, .enum_type = LDA, .uses_two_args = true},
	{.name = "ADD", .name_len = 3, .enum_type = ADD, .uses_two_args = true},
	{.name = "SUB", .name_len = 3, .enum_type = SUB, .uses_two_args = true},
	{.name = "MUL", .name_len = 3, .enum_type = MUL, .uses_two_args = true}
};

static const struct REG_MATCH_INFO REGS[] = {
	{.name = "R1", .name_len = 2, .enum_type = R1},
	{.name = "R2", .name_len = 2, .enum_type = R2},
	{.name = "R3", .name_len = 2, .enum_type = R3},
	{.name = "R4", .name_len = 2, .enum_type = R4},
	{.name = "RBP", .name_len = 3, .enum_type = RBP},
	{.name = "RSP", .name_len = 3, .enum_type = RSP},
	{.name = "RIP", .name_len = 3, .enum_type = RIP}
};


/*
	please is there a better way to do this than having two args,
	waste of space for one arg instructions
	im not good at C
*/

static char *srcp_cur, *srcp_start;

// strncmp but worse
static bool strcmp_with_lens(const char *str1, const char *str2,
  const int str1_len, const int str2_len)
{
	if (str1_len != str2_len) return false;
	int i = 0;
	while (*str1++ == *str2++ && i++ < str1_len);	// arbitrary, could've chosen str2_len
	return i == str1_len;
}

static inline bool is_whitespace(char c)
{
	return (c == ' ' || c == '\n' || c == '\t'
	  || c == '\v' || c == '\f' || c == '\r');	// do i need to acount for '\v', '\f', or '\r' lol?
}

static char *get_wordp(int *word_len)	// prob need to increment to skip delimiters
{
	while (is_whitespace(*srcp_cur))
		srcp_cur++;
	char *srcp_offset = srcp_cur;
	int debug_pos = (int) (srcp_cur - srcp_start);

	while (!is_whitespace(*srcp_cur) && *srcp_cur != ';') {
		if (*srcp_cur == '\0') {
			ERREXIT("Unterminated word beginning at pos %d\n",
			  debug_pos);
		}
		srcp_cur++;
	}
	*word_len = (int) (srcp_cur - srcp_offset);
	return srcp_offset;
	
}

static inline bool is_str_int_with_len(char *str, int len)
{
	char *end = str + len;
	if (*str == '-')	// account for negative sign
		str++;
	while (str < end && (*str >= '0' && *str <= '9'))
		str++;
	return str == end;
}

static int8_t str_to_int8_t_with_len(char *str, int len)
{
	bool is_negative = *str == '-';	// unary minus
	if (len > ((is_negative) ? 4 : 3))	// Ex: 2552,	takes more than 8 bits, len + 1 to account for sign
		ERREXIT("8-bit integer has %d digits which exceeds the limit of 3"
		  " at pos %d\n", len, (int) (str - srcp_start));
	int exp = 1;
	if (is_negative)
		str++, len--, exp = -1;
	int sum = 0;
	char *int_start = str + len - 1;
	while (int_start >= str) {
		printf("AAA: %c\n", *int_start);
		sum += (*int_start - '0') * exp;
		int_start--, exp *= 10;
	}
	if (sum > INT8_MAX)
		ERREXIT(
		  "8-bit integer has value of %d which exceeds the limit of 127"
		  " at pos %d\n", sum, (int) (str - srcp_start));
	else if (sum < INT8_MIN)
		ERREXIT(
		  "8-bit integer has value of %d which exceeds the limit of -128"
		  " at pos %d\n", sum, (int) (str - srcp_start));
	return (int8_t) sum;
}

static void set_instr_type_info(struct Instr *instr,
  char *instr_name, int instr_name_len, bool *has_two_args)
{
	// i am most likely going to replace this with a hashmap
	static const int INSTRS_COUNT = (int)	// cast size_t to int
	  (sizeof INSTRS / sizeof *INSTRS);
	for (int i = 0; i < INSTRS_COUNT; i++) {
		if (strcmp_with_lens(instr_name,
		  INSTRS[i].name, instr_name_len, INSTRS[i].name_len)) {
			instr->type = INSTRS[i].enum_type;
			*has_two_args = INSTRS[i].uses_two_args;
			return;
		}
	}
	ERREXIT( "Invalid instruction at pos %d\n",
	  (int) (instr_name - srcp_start));
}

static void set_instr_arg_info(struct Instr *instr,
  int which_arg, char *arg, int arg_len, bool reg_only)
{
	if (!reg_only) {
		if (is_str_int_with_len(arg, arg_len)) {
			int8_t arg_value = str_to_int8_t_with_len(arg, arg_len);
			if (which_arg == 1) {
				instr->arg1_type = INT8;
				instr->arg1.int8 = arg_value;
			}
			else {
				instr->arg2_type = INT8;
				instr->arg2.int8 = arg_value;
			}
			return;	
		}
	}
	static const int REGS_COUNT = (int) (sizeof REGS / sizeof *REGS);

	for (int i = 0; i < REGS_COUNT; i++) {
		if (strcmp_with_lens(arg, REGS[i].name,
		  arg_len, REGS[i].name_len)) {
			if (which_arg == 1) {
				instr->arg1_type = REG;
				instr->arg1.reg = REGS[i].enum_type;
			}
			else {
				instr->arg2_type = REG;
				instr->arg2.reg = REGS[i].enum_type;
			}
			return;
		}
	}
	(reg_only) ?
	(ERREXIT("Expected register argument at pos %d\n",
	  (int) (arg - srcp_start))) :
	(ERREXIT("Expected register or 8-bit integer argument at pos %d\n",
	  (int) (arg - srcp_start)));
}

static inline void end_statement(int debug_statement_pos)
{
	while (is_whitespace(*srcp_cur))	// consume trailing whitespace
		srcp_cur++;
	if (*srcp_cur != ';')
		ERREXIT("Expected semicolon ';' to end"
		  " statement at pos %d\n",
		  (int) (srcp_cur - srcp_start));
	srcp_cur++;
}

static void ___debug_instr(struct Instr *instr)
{
	printf("--DEBUG INSTR:--\n");
	printf("Type: %d\n", instr->type);
	printf("Arg1 type: %d\n", instr->arg1_type);
	printf("Arg1 value: %d\n",
	  instr->arg1_type == REG ? (int) instr->arg1.reg : instr->arg1.int8);
	
	switch (instr->type) {
		case PUSH: case POP: case INC: case DEC:
			break;
		default:
			printf("Arg2 type: %d\n", instr->arg2_type);
			printf("Arg2 value: %d\n",
			  instr->arg2_type == REG ? (int) instr->arg2.reg : instr->arg2.int8);
	}
}

// gonna add a dynamically resizing array to contain instr struct elements
static void parse_src(char src[], int src_len,
  struct Instr *parsed_instrs, int *instrs_len)
{
	srcp_cur = srcp_start = src; // only using a pointer to a local variable during it's lifetime, i think it's fine
	while(*srcp_cur != '\0') {	// will change to compare pointers
		struct Instr instr;
		int instr_name_len = 0;
		char *instr_name = get_wordp(&instr_name_len);

		if (instr_name_len == 0) {
			ERREXIT("Expected instruction at pos %d\n",
			  (int) (srcp_cur - srcp_start));
		}
			
		int statement_start_pos = (int) (instr_name - srcp_start);
		bool has_two_args = false;
		
		set_instr_type_info(&instr, instr_name,
		  instr_name_len, &has_two_args);
		
		int arg1_len;
		char *arg1 = get_wordp(&arg1_len);
		bool arg1_is_reg_only = (instr.type == PUSH) ? false : true;

		if (has_two_args) {
			int arg2_len;
			char *arg2 = get_wordp(&arg2_len);

			if (arg1_len == 0) {
				ERREXIT(
				  "Expected 2 args for instruction starting at pos %d\n",
				  statement_start_pos);
			}

			if (arg2_len == 0) {
				ERREXIT(
				  "Expected arg 2 for instruction starting at pos %d\n",
				  statement_start_pos);
			}

			set_instr_arg_info(&instr, 1, arg1,
			  arg1_len, arg1_is_reg_only);
			set_instr_arg_info(&instr, 2, arg2,
			  arg2_len, false);
		}
		else {
			if (arg1_len == 0) {
				ERREXIT(
				  "Expected arg for instruction starting at pos %d\n",
				  statement_start_pos);
			}
			set_instr_arg_info(&instr, 1, arg1,
			  arg1_len, arg1_is_reg_only);
		}

		end_statement(statement_start_pos);
		___debug_instr(&instr);
	}
}
 

// testing
int main(void)
{
	char src[] =
	" POP   R2 ;"
	"ADD R1 123; INC R2;";

	int src_len = sizeof src;	// byte is guaranteed to be 1 byte, no need to divide by an element's type size
	int instrs_len;
	struct Instr PLACE_HOLDER = {.type = NOP};
	struct Instr *parsed_instrs = &PLACE_HOLDER;

	parse_src(src, src_len, parsed_instrs, &instrs_len);
	return 0;
}
