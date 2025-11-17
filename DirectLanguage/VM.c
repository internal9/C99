#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <limits.h>
#include <errno.h>

#if CHAR_BIT != 8
	#error "DVM: a byte must be 8 bits for VM operations to work as intended"
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

enum OPCODE
{
	// memory transfer operations
	MOVR,	// # bytes used depends on reg used
	MOVS	// move value into stack address
	MOVH	// move value into heap address at stack address?
	TRS,	// transfer to reg from stack
	TSR,	// transfer to stack from reg
	TRH,	// transfer to reg from heap address
	THR,	// transfer to heap address from reg

	// arithmetic operations
	// op [reg]
	ZERO,
	INC,
	DEC,

	//op reg, [value | freg]
	ADD,
	SUB,
	MUL,
	DIV,
	SQRT,
	POW,
	LOG,

	// bitwise operations
	// op reg, [value | reg]
	NOT,
	AND,
	OR,
	XOR,
	BIC,

	// logical shift (no regard for sign bit)
	LLSH,
	LRSH,

	// arithmetic shift (sign bit unaffected)
	ALSH,
	ARSH,

	// stack dedicated operations
	// op [val | reg]
	PUSH,
	FPUSH,

	// op [reg]
	POP,
	FPOP,	// popping into a general register performs double -> u64 conversion

	// branching operations
	JMP,
	JNE,
	JE,
	JL,
	JLE,
	JG,
	JGE,
	CALL,
	BICALL,	// built-in call
	RET,

	// dynamic memory operations
	/*
		alloc [addr], [bytes] heap-array addr to put memory address of heap-allocated data into
		has mode bit for zero-initializing newly allocated data IF resizing bigger
	*/
	ALLOC,
	/*
		realloc [mem_addr], [bytes]
		mem_addr: addr of the heap-allocated data itself
	*/
	REALLOC,
	/*
		dealloc [mem_addr]
		mem_addr: addr of the heap-allocated data itself
	*/
	DEALLOC,

	// misc operations
	CMP,
};

// should just remove lol
// Specify size to use for operands that can either be regs or literals, to save bytes ig. Orrr just read a reg
enum OPERAND_READ_MODE
{
	USE_REG,
	
	/*
		If instructions operates on reg or stack location:
			 They expect sizeof(int64_t) or sizeof(double) amount of bits,
			 *depending* on if a reg or floating-point reg is being used
	
		If instruction can use literals:
			They expect sizeof(int64_t) or sizeof(double) amount of bits
			*depending* on instruction
	*/
	FULL,
	HALF,
	EIGHT
};

enum REG
{	
	R1,
	R2,
	RAX,
	RBP,
	PRBP,
	RSP,
	FR1,
	FR2,
};

#define ASSERT_IS_REG(int_val, err_msg) ((int_val > FR2) ? (ERREXIT(err_msg), 0) : int_val)	// yeah idk about this
#define IS_I64_REG(int_val, err_msg) ((int_val >= R1 && int_val <= PRBP) ? (ERREXIT(err_msg), 0) : int_val)
#define IS_FP_REG(int_val, err_msg) ((int_val == FR1 || int_val == FR2) ? (ERREXIT(err_msg), 0) : int_val)

int64_t i64_regs[] = {
	[R1] = 0,
	[R2] = 0,
	[RAX] = 0,	
	[RBP] = 0,
	[PRBP] = 0,
	[RSP] = -1,
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
const int F_LITERAL_SIZES[] = {
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
static inline uint8_t expect_byte(unsigned long int p_bytes_index, const char *err_msg)
{
	// max 'p_bytecode' index = file_size - 1
	if (p_bytes_index >= file_size)
		ERREXIT(err_msg);
	return p_bytecode[p_bytes_index];
}

// should bound check 'count'
static inline uint8_t *expect_bytes(unsigned long int p_bytes_index, int count, const char *err_msg)	// mainly just checks if specified amount of bytes exist, not literally allocate, idk
{
	if (count < 2)
		ERREXIT("expect_bytes: Expected 'count' >= 2");
	
	if ((p_bytes_index + count - 1) >= file_size)
		ERREXIT(err_msg, count, (unsigned long) (p_bytes_index - (file_size - 1)));	// 3rd arg for insufficient byte count that 'err_msg' must log
	return p_bytecode + p_bytes_index;
}

/*
static inline enum REG_SPEC expect_reg_spec(unsigned long int p_bytes_index, const char *err_msg)
{
	unsigned int byte = expect_byte(p_bytes_index, err_msg);	// scared of usual arithmetic conversion misinterpreting as negative values
	if (byte > LAST_REG_SPEC)
		ERREXIT(err_msg, byte);	// 'err_msg' must handle byte value
	return (enum REG_SPEC) byte;
}
*/

// surprised memcpy doesn't take a char pointer
static inline void expect_bytes_memcpy(void *dest, unsigned long int p_bytes_index, int count, const char *err_msg)
{
		uint8_t *src_literal_bytes = expect_bytes(ip, count, err_msg);	// one again, 'long' for insufficient byte count that 'err_msg' must log
		memcpy(dest, src_literal_bytes, count);
}

// might just change these into opcode funcs
static void stack_push(uint8_t byte)
{
	if (i64_regs[RSP] == STACK_SIZE - 1)
		ERREXIT("VM stack overflow\n");

	stack[++i64_regs[RSP]] = byte;
}

static void stack_pushn(uint8_t *p_bytes, int count)
{
	if (i64_regs[RSP] == STACK_SIZE - count)
		ERREXIT("VM stack overflow\n");

	i64_regs[RSP] += count;
	memcpy((stack + ip) + 1, p_bytes, (size_t) count);
}

// instrs
static void op_movr(enum OPERAND_READ_MODE operand_2_read_mode)
{
	// Max of 16 registers can be represented, only 8 exist so far
	uint8_t info_byte = expect_byte();
	enum REG dest_reg = ASSERT_IS_REG(info_byte & 0xF0);		// 0xF0 = 0b11110000, 16 possible registers that can be implemented

	if (operand_2_read_mode == USE_REG)
	{
		enum REG src_reg = ASSERT_IS_REG(info_byte & 0x0F);		// 0xF0 = 0b00001111
		if (IS_I64_REG(dest_reg))
			// ADD RANGE CASTING!!!! since double is being converted to int64_t
			i64_regs[dest_reg] = IS_64_REG(src_reg) ? i64_regs[src_reg] : try_cast_fp_to_i64(fp_regs[src_reg]);
		else
			fp_regs[dest_reg] = IS_64_REG(src_reg) ? (double) i64_regs[src_reg] : fp_regs[src_reg];
	}
	else	// literal size specification
	{
		int literal_type_size;

		if (IS_I64_REG(dest_reg))
		{
			int64_t src_literal;
			literal_type_size = I_LITERAL_SIZES[operand_2_read_mode];
			expect_bytes_memcpy(&src_literal, ip, literal_type_size, "some err msg lmaoooo");
			i64_regs[dest_reg] = src_literal;
		}
		else
		{
			double src_literal;
			literal_type_size = FP_LITERAL_SIZES[operand_2_read_mode];
			expect_bytes_memcpy(&src_literal, ip, literal_type_size, "some err msg lmaoooo");
			fp_regs[dest_reg] = src_literal;
		}
	}
}

static void run_bytecode(void)
{
	printf("INSTR POINTER: %lu\n FILE SIZE: %lu\n", ip, file_size);
	while (ip != file_size)	// accomodate if ip points one past 'p_bytes'
	{
		uint8_t byte = p_bytecode[ip];
		int opcode = byte & 0x3F	// 0b00111111
		int operand_read_mode = byte & 0xFC	// 0b11000000
		printf("HEX BYTE: %.2X\n", byte);
	
		switch (opcode)
		{
			case MOVR: op_movr(operand_read_mode); break;
		}
	}
}

static void init_bytecode(FILE *p_bytecode_file)
{
	if (fseek(p_bytecode_file, 0, SEEK_END) != 0)
		PERREXIT("Failed to read bytecode file: ");

	long ftell_result = ftell(p_bytecode_file);
	if (ftell_result == -1L)
		PERREXIT("Failed to read bytecode file: ");	// Especially file size that caused an overflow

	file_size = (unsigned long) ftell_result;	// i hate this

	if (file_size == (long) 0)
		return;

	if (fseek(p_bytecode_file, 0, SEEK_SET) != 0)	// heard setting it to start is safe, i'm paranoid tho
		PERREXIT("Failed to read bytecode file: ");

	p_bytecode = malloc((size_t) file_size); // I would use a VLA but I can't gracefully handle those errors if a stack overflow happens
		PERREXIT("Failed to read bytecode file: ");

	// assuming bytecode files don't have an EOF indicator (Linux)
	if (fread(p_bytecode, 1, (size_t) file_size, p_bytecode_file) != (size_t) file_size)	// if only file_size was size_t instead of long..
		PERREXIT("Failed to read bytecode file: ");
	
	// debug
	printf("Bytecode file size: %lu\n", file_size);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		ERREXIT("Expected only one argument, either -i (print info), or a bytecode file\n");

	if (strcmp(argv[1], "-i"))
	{
		printf("Some useful info:\n"
			"sizeof(double): %zu\n"
			"sizeof(int64_t): %zu\n",
			"sizeof(void*): %zu\n",
			sizeof(double), sizeof(int64_t), sizeof(void*));
		return EXIT_SUCCESS;
	}
	// FUN FACT: stderr is typically unbuffered, it prints immediately due to the importance of warning and error messages!

	const char *file_name = argv[1];
	FILE *p_bytecode_file = fopen(file_name, "rb");
	if (p_bytecode_file == NULL)	// FUN FACT: NULL is implementation-defined, it could be integer literal 0 or (void*) 0, either way it always behaves consitently
		PERREXIT("Failed to open file '%s': ", file_name);

	init_bytecode(p_bytecode_file);
	run_bytecode();
	return EXIT_SUCCESS;
}
