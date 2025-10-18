#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>

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
// MOV REG
	MOVB1RV = 1,
	MOVB8RV = 2,

	MOVB1RR = 1,
	MOVB8RR = 2,
	
	MOVB1RS = 3,
	MOVB8RS = 4,

// MOV STACK
	MOVB1SV,
	MOVB1SS,

	MOVB8SV,
	MOVB8SS,

// INC STACK
	INCB1SV,
	INCB1SS,

	INCB8SV,
	INCB8SS,

// ADD STACK
	ADDB1SV,		// ADD value to value located on stack address
	ADDB1SS,

	ADDB8SV,
	ADDB8SS,

// SUB STACK
	SUBB1SV,
	SUBB1SS,

	SUBB8SV,
	SUBB8SS,

// DEC STACK
	DECB1SV,
	DECB1SS,

	DECB8SV,
	DECB8SS,

// PUSH
	PUSHB1V,
	PUSHB1S,
	POPB1R,
	POPB8R,

	PUSHB8V,
	PUSHB8S,

// UNCONDITIONAL JMP
	JMPFV,	// JMP forwards of literal offset count
	JMPFR,	// JMP forwards of register value count
	JMPBV,	// JMP backwards of literal offset count

// CMP
	CMPRV,
	CMPSV,

// CONDITIONAL FORWARD JMP
	JEFV,
	JNEFV,

	JGFV,
	JGEFV,

	JLFV,
	JLEFV,

// CONDITIONAL BACKWARD JMP
	JEBV,
	JNEBV,

	JGBV,
	JGEBV,

	JLBV,
	JLEBV,

// IDK
	CALL,	// implement via jmps?
	RET,
};

enum REG
{
	RAX,
	R1,
	R2,
	R3,
	R4,
	RBP,
	RSP,
};

int64_t regs[] = {
	[RAX] = 0,
	[RBP] = 0,
	[RSP] = -1,
};

static uint8_t stack[STACK_SIZE]; // full stack implementation, 'ip' points to most recently pushed byte

static unsigned long int ip = 0;	// although this is implicitly initialized to 0, doing so explicitly is good practice
static unsigned long int file_size;
static uint8_t *p_bytecode;


static inline uint8_t expect_byte(const char *err_msg, unsigned long int p_bytes_index)
{
	if (ip >= file_size)
	{
		if (err_msg != NULL)
			fprintf(stderr, "%s\n", err_msg);
		exit(EXIT_FAILURE);
	}
	return p_bytecode[p_bytes_index];
}

#define expect_bytes(p_bytes_index, err_msg_format, ...) \
	((p_bytes_index >= file_size) ? \
		(fprintf(stderr, err_msg_format, __VA_ARGS__), exit(EXIT_FAILURE), NULL) : \
		(p_bytecode + p_bytes_index)) \

static void stack_push(uint8_t byte)
{
	if (regs[RSP] == STACK_SIZE - 1)
	{
		fprintf(stderr, "VM stack overflow\n");
		exit(EXIT_FAILURE);
	}

	stack[++regs[RSP]] = byte;
}

static void stack_pushn(uint8_t *p_bytes, int count)
{
	if (regs[RSP] == STACK_SIZE - count)
	{
		fprintf(stderr, "VM stack overflow\n");
		exit(EXIT_FAILURE);
	}

	regs[RSP] += count;
	memcpy((stack + ip) + 1, p_bytes, (size_t) count);
}

static void op_movb1rv(void)
{
	static const int INSTR_SIZE = 3;
	uint8_t	reg_dest = expect_byte("Expected dest reg for instr 'movb1rv'", ip + 1);
	regs[reg_dest] = expect_byte("Expected '1' byte for instr 'movb1rv'", ip + 2);
	ip += (unsigned long int) INSTR_SIZE;
}

static void op_movb8rv(void)
{
	static const int INSTR_SIZE = 10;
	uint8_t	reg_dest = expect_byte("Expected dest reg for instr 'movb8rv'", ip + 1);
	uint8_t *p_bytes = expect_bytes(8,
		"Expected '8' bytes for instr 'movb8rv', %d\n",
		(int) (file_size - (ip + 1)));
	memcpy(regs + reg_dest, p_bytes, 8);
	ip += (unsigned long int) INSTR_SIZE;
}

static void op_movb1rr(void)
{
	static const int INSTR_SIZE = 3;
	uint8_t	reg_dest = expect_byte("Expected dest reg for instr 'movb1rr'", ip + 1);
	uint8_t	reg_src = expect_byte("Expected src reg for instr 'movb1rr'", ip + 1);

	// error handle lol
	regs[reg_dest] = regs[reg_src] & (int64_t) 0xFF00000000000000;
	ip += (unsigned long int) INSTR_SIZE;
}

static void op_movb1rst(void)
{
	static const int INSTR_SIZE = 3;
	uint8_t	reg_dest = expect_byte("Expected dest reg for instr 'movrr'", ip + 1);
	uint8_t	reg_src = expect_byte("Expected src reg for instr 'movrr'", ip + 1);

	// error handle lol
	regs[reg_dest] = regs[reg_src] & (int64_t) 0xFF00000000000000;
	ip += (unsigned long int) INSTR_SIZE;
}

static void op_pushvb1(void)
{
	static const int INSTR_SIZE = 2;
	uint8_t byte = expect_byte("Expected '1' byte for instr 'pushvb1'", ip + 1);
	stack_push(byte);
	ip += (unsigned long int) INSTR_SIZE;
}

static void op_pushvb8(void)
{
	static const int INSTR_SIZE = 9;
	/* if (file_size - (ip + 1) < 8)
	{
		fprintf(stderr, "Expected '8' bytes for instr 'pushvb8', instead got '%d' bytes",
			(int) (file_size - (ip + 1)));
	}
	uint8_t *bytes = (p_bytes + ip) + 1; */

	uint8_t *p_bytes =
		expect_bytes((ip + 1) + 8, "Expected '8' bytes for instr 'pushvb8', instead got '%d' bytes",
		(int) (file_size - (ip + 1)));

	stack_pushn(p_bytes, 8);
	ip += (unsigned long int) INSTR_SIZE;
}

// Jumps have no need to increment ip by their instr size since the point is to jump to a specified valid offset
static void op_jmpfv(void)
{
	/* if (file_size - (ip + 1) < 4)
	{
		fprintf(stderr,
			"Expected 4-byte unsigned integer for offset for instr 'jmpfv', instead got %d bytes\n",
			(int) (file_size - (ip + 1)));
		exit(EXIT_FAILURE);
	} */

	/*
		!!! THIS ASSUMES THAT INTS ARE STORED IN LITTLE ENDIAN IN THE BYTECODE FILE!!!
		(LSB stored at lowest mem addr) (might add big endian support via conditional compilation)
	*/
	int offset; // should offset bet a long instead?
	memcpy(&offset, p_bytecode + ip + 1, (size_t) 4);	// '+ 1' so ip points to first byte

	if (offset == 0)
	{
		fprintf(stderr, "Jump offset cannot be '0'\n");
		exit(EXIT_FAILURE);		
	}

	if (offset < 5)
	{
		fprintf(stderr, "Offset of '%d' for instr 'jmpfv' is less than 5 and"
			" would've jumped to byte part of offset integer\n", offset);
		exit(EXIT_FAILURE);
	}

	if (ip > ULONG_MAX - (unsigned long int) offset)
	{
		fprintf(stderr, "Offset of '%d' for instr 'jmpfv' will cause overflow" 
			"for instruction pointer\n", offset);
		exit(EXIT_FAILURE);
	}
	
	ip += (unsigned long int) offset;
	if (ip > file_size - 1)
	{
		fprintf(stderr, "Instruction pointer has value of '%lu' after adding offset"
			" of '%d' for instr 'jmpfv' which is greater than the last byte offset of %lu\n",
			ip, offset, file_size - 1);
		exit(EXIT_FAILURE);
	}

	printf("BYTE POST JUMP: %X\n", p_bytecode[ip]);
}

static void op_jmpfr(void)
{
	
}


static void op_jmpbv(void)
{
/*	if (file_size - (ip + 1) < 4)
	{
		fprintf(stderr,
			"Expected 4-byte integer for offset for instr 'jmpbv', instead got %d bytes\n",
			(int) (file_size - (ip + 1)));
		exit(EXIT_FAILURE);
	} */

	uint8_t *p_offset_int = expect_bytes((ip + 1) + 4,
		"Expected 4-byte integer for instr 'jmpbv', instead got %d bytes\n",
		(int) (file_size - (ip + 1)));

	int offset; // should offset bet a long instead?
	memcpy(&offset, p_offset_int, (size_t) 4);

	if (offset == 0)
	{
		fprintf(stderr, "Jump offset cannot be '0'\n");
		exit(EXIT_FAILURE);		
	}

	if (ip < (unsigned long int) offset)
	{
		fprintf(stderr, "Offset of '%d' for instr 'jmpbv' will cause underflow for instruction pointer\n", offset);
		exit(EXIT_FAILURE);
	}	

	ip -= (unsigned long int) offset;
}

static void op_jmpbr(void)
{
	
}

static void op_pushv(void)
{
	static const int INSTR_SIZE = 2;

	if (ip == file_size - 1)	// 'ip' currently points to the 'pushv' byte
	{
		fprintf(stderr, "Expected '1' byte for instr 'pushv', got '0' bytes instead\n");
		exit(EXIT_FAILURE);
	}

	printf("IP V SIZE: %lu %lu\n", ip, file_size);
	uint8_t byte = p_bytecode[ip + 1];
	if (regs[RSP] == STACK_SIZE - 1)
	{
		fprintf(stderr, "VM stack overflow (max stack size of '%d')\n", STACK_SIZE);
		exit(EXIT_FAILURE);
	}

	stack[++regs[RSP]] = byte;
	ip += (unsigned long int) INSTR_SIZE;
}

static void run_bytecode(void)
{
	printf("INSTR POINTER: %lu\n FILE SIZE: %lu\n", ip, file_size);
	while (ip != file_size)	// accomodate if ip points one past 'p_bytes'
	{
		uint8_t byte = p_bytecode[ip];
		printf("HEX BYTE: %.2X\n", byte);
		switch (byte)
		{
			case MOVB1RV: op_movb1rv(); break;
			case MOVB8RV: op_movb8rv(); break;
			
			// case JMPFV: op_jmpfv(); break;
			// case JMPBV: op_jmpbv(); break;
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

	p_bytecode = malloc((size_t) file_size); // I would use a VLA but I can't gracefully handle those errors if a stack overflow happens
	if (p_bytecode == NULL)
	{
		perror("Failed to read bytecode file");
		exit(errno);
	}

	// assuming bytecode files don't have an EOF indicator (Linux)
	if (fread(p_bytecode, 1, (size_t) file_size, p_bytecode_file) != (size_t) file_size)	// if only file_size was size_t instead of long..
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
