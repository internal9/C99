/*
	*NOTES*
	Make incrementing ip after running instruction as easy as possible
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>	// #includes <stdint.h> too
#include <math.h>
#include <limits.h>
#include <errno.h>

#if CHAR_BIT != 8
	#error "How?????"
#endif

// 'ERREXIT' & 'PERREXIT' expect *string literals* as the first argument format
#define ERREXIT(...) (fprintf(stderr, __VA_ARGS__), putc('\n', stderr), exit(EXIT_FAILURE))	// macro 'putc', yeah just don't have expressions wide side effects and you'lll be fine!!!!
/*
	Using fprintf to allow for formatting, which perror doesn't, perror appends newline char to stderr
	errmsg should end with ": " to accomodate for perror msg
*/
#define PERREXIT(...) (fprintf(stderr, __VA_ARGS__), fputs(": ", stderr), perror(NULL), exit(errno))
#define MAX(x, y) (((x) >= (y)) ? (x) : (y))
#define STACK_SIZE 512	// i sure do love arbitrary numbers

// Explicit values just to make stuff easier
enum OPCODE
{
	// memory transfer operations
	MOVR = 0,	// # bytes used depends on reg used
	MOVS = 1,	// move value into stack address
	MOVH = 2,	// move value into heap address at stack address?
	TRS = 3,	// transfer to reg from stack
	TSR = 4,	// transfer to stack from reg
	TRH = 5,	// transfer to reg from heap address
	THR = 6,	// transfer to heap address from reg

	// arithmetic operations
	// op [reg]
	ZERO = 6,
	INC = 7,
	DEC = 8,

	//op fpreg, [value | fpreg]
	ADD = 9,
	SUB = 10,
	MUL = 11,
	DIV = 12,
	SQRT = 13,
	POW = 14,
	LOG = 15,

	// bitwise operations
	// op reg, [value | reg]
	NOT = 16,
	AND = 17,
	OR = 18,
	XOR = 19,
	BIC = 20,

	// logical shift (no regard for sign bit)
	LLSH = 21,
	LRSH = 22,

	// arithmetic shift (sign bit unaffected)
	ALSH = 23,
	ARSH = 24,

	// stack dedicated operations
	// op [val | reg]
	PUSH = 25,
	FPUSH = 26,

	// op [reg]
	POP = 26,
	FPOP = 27,	// popping into a general register performs double -> u64 conversion

	// branching operations
	JMP = 28,
	JNE = 29,
	JE = 30,
	JL = 31,
	JLE = 32,
	JG = 33,
	JGE = 34,
	CALL = 35,
	BICALL = 36,	// built-in call
	RET = 37,

	// dynamic memory operations
	/*
		alloc [addr], [bytes] heap-array addr to put memory address of heap-allocated data into
		has mode bit for zero-initializing newly allocated data IF resizing bigger
	*/
	ALLOC = 38,
	/*
		realloc [mem_addr], [bytes]
		mem_addr: addr of the heap-allocated data itself
	*/
	REALLOC = 39,
	/*
		dealloc [mem_addr]
		mem_addr: addr of the heap-allocated data itself
	*/
	DEALLOC = 40,

	// misc operations
	CMP = 41,
	PRINTREG = 42,
};

// should just remove lol
// Specify size to use for operands that can either be regs or literals, to save bytes ig. Orrr just read a reg
enum OPERAND_READ_MODE
{
	USE_REG = 0,
	
	/*
		If instructions operates on reg or stack location:
			 They expect sizeof(int64_t) or sizeof(double) amount of bits,
			 *depending* on if a reg or floating-point reg is being used
	
		If instruction can use literals:
			They expect sizeof(int64_t) or sizeof(double) amount of bits
			*depending* on instruction
	*/
	FULL = 1,
	HALF = 2,
	EIGHT = 3
};

enum REG
{	
	R1 = 0,
	R2 = 1,
	RAX = 2,
	RBP = 3,
	PRBP = 4,
	RSP = 5,
	FR1 = 6,
	FR2 = 7
};

// wish i could change enum base type in C99
#define IS_I_REG(int_val) ((int) (int_val) >= (int) R1 && (int) (int_val) <= (int) PRBP)
#define IS_FP_REG(int_val) ((int) (int_val) == (int) FR1 || (int) (int_val) == (int) FR2)
#define IS_REG(int_val) ((int) (int_val) >= (int) R1 && (int) (int_val) <= (int) FR2)

int64_t i_regs[] = {
	[R1] = 0,
	[R2] = 0,
	[RAX] = 0,	
	[RBP] = 0,
	[PRBP] = 0,
	[RSP] = -1,	// full stack implementation, so RSP points to most recently pushed value started from 0
};

double fp_regs[] = {
	[FR1] = 0,
	[FR2] = 0,
};

const int I_LITERAL_SIZES[] = {
	[FULL] = 8,
	[HALF] = 4,
	[EIGHT] = 1,
};

// C Standard: floating-point size varies
const int FP_LITERAL_SIZES[] = {
	[FULL] = sizeof(double),
	[HALF] = MAX(sizeof(double) / 2, 1),
	[EIGHT] = MAX(sizeof(double) / 8, 1)
};

static uint8_t stack[STACK_SIZE]; // full stack implementation, 'ip' points to most recently pushed byte

static unsigned long ip = 0;	// although this is implicitly initialized to 0, doing so explicitly is good practice
static unsigned long file_size;
static uint8_t *p_bytecode;

// Util
#define FP_TO_I64_MAX ((double) (INT64_MAX - 100000))	// random arbitrary value cuz floating-point conversion errors are scary!!!
#define FP_TO_I64_MIN ((double) (INT64_MIN + 100000))

static inline int64_t try_cast_fp_to_i64(double fp_val)
{
	if (fp_val < FP_TO_I64_MIN || fp_val > FP_TO_I64_MAX || isnan(fp_val) || isinf(fp_val))
		ERREXIT("Could not cast floating-point of value '%f' to 64-bit signed integer", fp_val);

	return (int64_t) fp_val;
}

// Stuff
static inline uint8_t expect_byte(unsigned long p_bytes_index, const char *err_msg)
{
	// max 'p_bytecode' index = file_size - 1
	if (p_bytes_index >= file_size)
		ERREXIT(err_msg);
	return p_bytecode[p_bytes_index];
}

// 'err_msg' is expected to have two 'unsigned long' formats for count & insufficient bytes given (compared to expected)
static inline uint8_t *expect_bytes(unsigned long p_bytes_index, unsigned long count, const char *err_msg)	// mainly just checks if specified amount of bytes exist, not literally allocate, idk
{
	if (count < 2)
		ERREXIT("expect_bytes: Expected 'count' >= 2");

	if (p_bytes_index > file_size)
	   	ERREXIT(err_msg, count, 0);	// 0 bytes given

	unsigned long byte_count_end = p_bytes_index + (count - 1);
	if (byte_count_end >= file_size)
		ERREXIT(err_msg, count, file_size - p_bytes_index);	// 3rd arg for insufficient byte count that 'err_msg' must log, e.g. "only got X bytes"
	return p_bytecode + p_bytes_index;
}

// surprised memcpy doesn't take a char pointer
static inline void expect_bytes_memcpy(void *dest, unsigned long p_bytes_index, unsigned long count, const char *err_msg)
{
	uint8_t *src_literal_bytes = expect_bytes(p_bytes_index, count, err_msg);
	memcpy(dest, src_literal_bytes, (size_t) count);
}

static inline enum REG expect_reg(unsigned long p_bytes_index, const char *err_msg)
{
	uint8_t byte = expect_byte(p_bytes_index, err_msg);
	if (!IS_REG(byte))
	   ERREXIT(err_msg);
	return (enum REG) byte;
}

// might just change these into opcode funcs
static void stack_push(uint8_t byte)
{
	if (i_regs[RSP] == STACK_SIZE - 1)
		ERREXIT("VM stack overflow");

	stack[++i_regs[RSP]] = byte;
}

static void stack_pushn(uint8_t *p_bytes, int count)
{
	if (i_regs[RSP] == STACK_SIZE - count)
		ERREXIT("VM stack overflow\n");

	i_regs[RSP] += count;
	memcpy((stack + ip) + 1, p_bytes, (size_t) count);
}

// base + index * scale + displacement
static unsigned long read_stack_addr(uint8_t info_byte)
{
	enum OPERAND_READ_MODE base_read_mode = info_byte & 0xC0;		// 0xC0 = 0b11000000
	enum OPERAND_READ_MODE index_read_mode = info_byte & 0x30;		// 0x30 = 0b00110000
	enum OPERAND_READ_MODE scale_read_mode = info_byte & 0x0C;	   	// 0x0C = 0b00001100
	enum OPERAND_READ_MODE displacement_read_mode = info_byte & 0x03;	// 0x03 = 0b00000011

	int64_t base = 0, index = 0, scale = 1, displacement = 0;
	if (base_read_mode == USE_REG)
	{
		enum REG base_reg = expect_reg(ip, "Expected byte containing valid reg for reading base of stack address");
		if (!IS_I_REG(base_reg))
			ERREXIT("Stack address: expected integer reg (i reg)");

		base = i_regs[base_reg];
	}
	else		   // Size specification for literal operand
	{
		expect_bytes_memcpy(&base, ip, (unsigned long) I_LITERAL_SIZES[base_read_mode],
			"Stack address: expected %lu bytes for base value literal, instead got %lu bytes");
	}
}

// instrs
static void op_movr(enum OPERAND_READ_MODE operand_2_read_mode)
{
	enum REG dest_reg = expect_reg(ip + 1, "movr: expected byte containing valid dest reg");

	if (operand_2_read_mode == USE_REG)
	{
		enum REG src_reg = expect_reg(ip + 2, "movr: expected byte containing valid src reg");
		printf("movr: %d %d\n", dest_reg, src_reg);

		if (IS_I_REG(dest_reg))
			i_regs[dest_reg] = IS_I_REG(src_reg) ? i_regs[src_reg] : try_cast_fp_to_i64(fp_regs[src_reg]);
		else
			fp_regs[dest_reg] = IS_I_REG(src_reg) ? (double) i_regs[src_reg] : fp_regs[src_reg];

		ip += 3;	// info_byte + regs info byte
	}
	else	// literal size specification
	{
		int literal_type_size;

		if (IS_I_REG(dest_reg))
		{
			int64_t i_src_literal;
			literal_type_size = I_LITERAL_SIZES[operand_2_read_mode];
			printf("the j: %d\n", operand_2_read_mode);

			expect_bytes_memcpy(&i_src_literal, ip + 2, (unsigned long) literal_type_size,
				"movr: expected %lu bytes for integer src literal, instead got %lu");

			i_regs[dest_reg] = i_src_literal;
		}
		else
		{
			double fp_src_literal;
			literal_type_size = FP_LITERAL_SIZES[operand_2_read_mode];
			expect_bytes_memcpy(&fp_src_literal, ip + 2, (unsigned long) literal_type_size,
				"movr: expected %lu bytes for floating-point src literal, instead got %lu");

			fp_regs[dest_reg] = fp_src_literal;
		}
	}
}

//	stack addr
static void op_movs(enum OPERAND_READ_MODE operand_2_read_mode)
{
	
}

static inline void op_printreg(void)	// wow
{
	enum REG src_reg = expect_byte(ip + 1, "printreg: expected byte for src reg");
	printf("printreg %d: %" PRId64 "\n", src_reg, i_regs[src_reg]);
	ip += 2;
}

static void run_bytecode(void)
{
	printf("INSTR POINTER: %lu\nFILE SIZE: %lu\n", ip, file_size);
	while (ip != file_size)	// accomodate if ip points one past 'p_bytes'
	{
		uint8_t byte = p_bytecode[ip];
		int opcode = byte & 0x3F;	// 0x3F = 0b00111111
		enum OPERAND_READ_MODE operand_read_mode = (byte & 0xC0) >> 6;	// 0xC0 = 0b11000000, shift 6 to extract value without impact from trailing zeros
		printf("HEX BYTE: %.2X\nOPCODE: %.2X\n", byte, opcode);
	
		switch (opcode)
		{
			case MOVR: printf("THE BYTE %d\n", operand_read_mode); op_movr(operand_read_mode); break;
			case PRINTREG: op_printreg(); break;
		}
	}
}

static void init_bytecode(FILE *p_bytecode_file)
{
	if (fseek(p_bytecode_file, 0, SEEK_END) != 0)
		PERREXIT("Failed to read bytecode file");

	long ftell_result = ftell(p_bytecode_file);
	if (ftell_result == -1L)
		PERREXIT("Failed to read bytecode file");	// Especially file size that caused an overflow

	file_size = (unsigned long) ftell_result;	// i hate this

	if (file_size == (long) 0)
		return;

	if (fseek(p_bytecode_file, 0, SEEK_SET) != 0)	// heard setting it to start is safe, i'm paranoid tho
		PERREXIT("Failed to read bytecode file");

	p_bytecode = malloc((size_t) file_size); // I would use a VLA but I can't gracefully handle those errors if a stack overflow happens

	if (p_bytecode == NULL)
		PERREXIT("Failed to read bytecode file");

	// assuming bytecode files don't have an EOF indicator (Linux)
	if (fread(p_bytecode, 1, (size_t) file_size, p_bytecode_file) != (size_t) file_size)	// if only file_size was size_t instead of long..
		PERREXIT("Failed to read bytecode file");
	
	// debug
	printf("Bytecode file size: %lu\n", file_size);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		ERREXIT("Expected only one argument, either -i (print info), or a bytecode file");

	const char *arg = argv[1];
	if (strcmp(arg, "-i") == 0)
	{
		printf("Some useful info:\n"
			"sizeof(double): %zu\n"
			"sizeof(int64_t): %zu\n"
			"sizeof(void*): %zu\n",
			sizeof(double), sizeof(int64_t), sizeof(void*));
		return EXIT_SUCCESS;
	}
	// FUN FACT: stderr is typically unbuffered, it prints immediately due to the importance of warning and error messages!

	FILE *p_bytecode_file = fopen(arg, "rb");
	if (p_bytecode_file == NULL)	// FUN FACT: NULL is implementation-defined, it could be integer literal 0 or (void*) 0, either way it always behaves consitently
		PERREXIT("Failed to open file '%s'", arg);

	init_bytecode(p_bytecode_file);
	run_bytecode();
	return EXIT_SUCCESS;
}
