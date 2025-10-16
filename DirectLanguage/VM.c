#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

#define ABS(x) ((x) >= 0 ? x : -(x)

#define INT64_SIZE 8
#define STACK_SIZE 512	// i sure do love arbitrary numbers

/*
	TODO:
	* debugging info for specific lines and positions
*/
/*
	*base_instr_name suffix1 suffix2
	*suffix2 determines how something will be moved into something specified by suffix1
	*suffix1 info:
		r: reg
		v: value, such as a literal
*/

enum OPCODE
{
	MOVRR = 0,
	MOVRV = 1,
	MOVSR = 2,
	MOVSV = 3,

	JMPFV = 4,	// JMP forwards of literal offset count
	JMPFR = 5,	// JMP forwards of register value count
};

enum REG
{
	RBP,
	RSP,
};

long int regs[] = {
	[RBP] = 0,
	[RSP] = 0,
};

static uint8_t stack[STACK_SIZE]; // full stack implementation, 'ip' points to most recently pushed byte

static unsigned long int ip = 0;	// although this is implicitly initialized to 0, doing so explicitly is good practice
static unsigned long int file_size;
static uint8_t *p_bytes;

static uint8_t expect_byte(const char *err_msg)
{
	if (ip == file_size)
	{
		if (err_msg != NULL)
			fprintf(stderr, "%s\n", err_msg);
		exit(EXIT_FAILURE);
	}
	return p_bytes[ip++];
}

static void stack_push(uint8_t byte)
{
	if (regs[RSP] == STACK_SIZE - 1)
	{
		fprintf(stderr, "VM stack overflow\n");
		exit(EXIT_FAILURE);
	}

	stack[++regs[RSP]] = byte;
}

// wait.. why did i make this again?
static void op_movrr(void)
{
	// uint8_t	reg_reg = expect_byte("Expected source reg for instr 'movrr'");
	// uint8_t	reg_dest = expect_byte("Expected destination reg for instr 'movrr'");
	
}

static void op_pushvi(void)
{
	static char err_msg[] = "Expected byte %d for instr 'pushvi'";
	static const size_t err_msg_size = sizeof err_msg;

	for (int i = 0; i < INT64_SIZE; i++)
	{
		snprintf(err_msg, err_msg_size, "Expected byte %d for integer literal for instr 'pushvi'", i + 1);
		uint8_t byte = expect_byte(err_msg);
		stack_push(byte);
	}
}

static void op_jmpfv(void)
{
	if (file_size - ip < 4)
	{
		fprintf(stderr,
			"Expected 4-byte unsigned integer for offset for instr 'jmpv', instead got %d bytes\n",
			(int) (file_size - ip));
		exit(EXIT_FAILURE);
	}

	/*
	// !!! THIS ASSUMES LITTLE ENDIAN!!! (LSB stored at lowest mem addr) (might add big endian support via conditional compilation)
	int offset = 0x00000000
		| expect_byte(NULL) << 24	// value 1 byte is usually represented by 4 hexadecimal digits
		| expect_byte(NULL) << 16
		| expect_byte(NULL) << 8
		| expect_byte(NULL)
	*/

	/*
		!!! THIS ASSUMES THAT INTS ARE STORED IN LITTLE ENDIAN IN THE BYTECODE FILE!!!
		(LSB stored at lowest mem addr) (might add big endian support via conditional compilation)
	*/
	int offset; // should offset bet a long instead?
	memcpy(&offset, p_bytes + ip, (size_t) 4);

	if (offset < 4)
	{
		fprintf(stderr, "Offset of '%d' for instr 'jmpv' is less than 4 and"
			" would've jumped to byte part of offset integer\n", offset);
		exit(EXIT_FAILURE);
	}

	if (ip > ULONG_MAX - (unsigned long int) offset)
	{
		fprintf(stderr, "Offset of '%d' for instr 'jmpv' will cause overflow" 
			"for instruction pointer\n", offset);
		exit(EXIT_FAILURE);
	}
	
	ip += (unsigned long int) offset;
	if (ip > file_size - 1)
	{
		fprintf(stderr, "Instruction pointer has value of '%lu' after adding offset"
			" of '%d' for instr 'jmpv' which is greater than last byte offset of %lu\n",
			ip, offset, file_size - 1);
		exit(EXIT_FAILURE);
	}

	printf("BYTE POST JUMP: %X\n", p_bytes[ip]);
}

static void op_jmpfr(void)
{
	
}

static void op_jmpbv(void)
{
	
}

static void op_jmpbr(void)
{
	if (file_size - ip < 4)
	{
		fprintf(stderr,
			"Expected 4-byte integer for offset for instr 'jmpv', instead got %d bytes\n",
			(int) (file_size - ip));
		exit(EXIT_FAILURE);
	}

	int offset; // should offset bet a long instead?
	memcpy(&offset, p_bytes + ip, (size_t) 4);
	
	if (offset == 0) return;
	if (ip < (unsigned long int) offset)
	{
		fprintf(stderr, "Offset of '%d' for instr 'jmpv' will cause underflow for instruction pointer\n", offset);
		exit(EXIT_FAILURE);
	}	

	ip -= (unsigned long int) offset;
}

static void run_bytecode(void)
{
	printf("INSTR POINTER: %lu\n FILE SIZE: %lu\n", ip, file_size);
	while (ip != file_size)
	{
		uint8_t byte = expect_byte(NULL);
		printf("HEX BYTE: %X\n", byte);
		switch (byte)
		{
			case MOVRR: op_movrr(); break;
			case JMPFV: op_jmpfv(); break;
		}
	}
}

static void init_bytecode(FILE *p_bytecode_file)
{
	if (fseek(p_bytecode_file, 0, SEEK_END) != 0)
	{
		perror("Failed to read bytecode file");
		exit(errno);
	}

	long int ftell_result = ftell(p_bytecode_file);
	if (ftell_result == -1L)
	{
		perror("Failed to read bytecode file");	// Especially file size that caused an overflow
		exit(errno);
	}

	file_size = (unsigned long int) ftell_result;	// i hate this

	if (file_size == (long int) 0)
		return;

	if (fseek(p_bytecode_file, 0, SEEK_SET) != 0)	// heard setting it to start is safe, i'm paranoid tho
	{
		perror("Failed to read bytecode file");
		exit(errno);
	}

	p_bytes = malloc((size_t) file_size); // I would use a VLA but I can't gracefully handle those errors if a stack overflow happens
	if (p_bytes == NULL)
	{
		perror("Failed to read bytecode file");
		exit(errno);
	}

	// assuming bytecode files don't have an EOF indicator (Linux)
	if (fread(p_bytes, 1, (size_t) file_size, p_bytecode_file) != (size_t) file_size)	// if only file_size was size_t instead of long..
	{
		perror("Failed to read bytecode file");
		exit(errno);
	}
	
	printf("Bytecode file size: %lu\n", file_size);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "Expected one bytecode file to run.\n");	// FUN FACT: stderr is typically unbuffered, it prints immediately due to the importance of warning and error messages!
		return EXIT_FAILURE;
	}

	const char *file_name = argv[1];
	FILE *p_bytecode_file = fopen(file_name, "rb");
	if (p_bytecode_file == NULL)	// FUN FACT: NULL is implementation-defined, it could be integer literal 0 or (void*) 0, either way it always behaves consitently
	{
		fprintf(stderr, "Failed to open file '%s': ", file_name);
		perror(NULL);
		return errno;
	}

	init_bytecode(p_bytecode_file);
	run_bytecode();
	return EXIT_SUCCESS;
}
