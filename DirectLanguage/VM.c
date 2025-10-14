#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#define STACK_SIZE 512	// i sure do love arbitrary numbers

/*
	*base_instr_name suffix1 suffix2
	*suffix2 determines how something will be moved into something specified by suffix1
	*suffix1 info:
		r: reg
		v: value, such as a literal
*/

enum OPCODE
{
	MOVRR,
	MOVRV,
	MOVSR,
	MOVSV,

		
};

enum REG
{
	RBP,
	RSP,
};

uint8_t regs[] = {
	[RBP] = 0,
	[RSP] = 0,
};

static uint8_t stack[STACK_SIZE]; 

static long int ip = 0;	// although this is implicitly initialized to 0, doing so explicitly is good practice
static long int file_size;
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

static void op_movrr()
{
	uint8_t	src_reg = expect_byte("Expected source reg for instr 'movrr'");
} 

static void run_bytecode(void)
{
	while (ip != file_size)
	{
		uint8_t byte = expect_byte(NULL);
		switch (byte)
		{
			case MOVRR: op_movrr(); break;
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

	file_size = ftell(p_bytecode_file);
	if (file_size == (long int) -1)
	{
		perror("Failed to read bytecode file");
		exit(errno);
	}

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
	
	printf("Bytecode file size: %ld\n", file_size);
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
