#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#define FILE_BUF_SIZE 1024	// power of 2 for alignment, thus performance

enum Opcode
{
	MOV,
	ADD,
};

static void run_bytecode(FILE *p_bytecode_file)
{
	uint8_t file_buf[FILE_BUF_SIZE];
	int bytes_read = 0;
	
	while ((bytes_read =
	  (int) fread(file_buf, 1, FILE_BUF_SIZE, p_bytecode_file)) > 0)	// size_t will be less tan or eq to 1024, so it's safe to cast to int
	{

		// TEST
		printf("bytes read: %d\n", bytes_read);
		for (int i = 0; i < bytes_read; i++)
			printf("HEX BYTE: %X\n", file_buf[i]);
	}

	if (ferror(p_bytecode_file))
	{
		perror("Error reading bytecode file");
		exit(errno);
	}
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

	run_bytecode(p_bytecode_file);
	return EXIT_SUCCESS;
}
