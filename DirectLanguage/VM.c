#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>

#define FILE_BUF_SIZE 1024	// power of 2 for alignment, thus performance

/*
	*base_instr_name suffix1 suffix2
	*suffix2 determines how something will be moved into something specified by suffix1
	*suffix1 info:
		r: reg
		v: value, such as a literal
*/

enum Opcode
{
	MOVRR,
	MOVRV,
	MOVSR,
	MOVSV,

		
};

static void run_bytecode(FILE *p_bytecode_file)
{
	uint8_t *p_bytes = NULL;
	uint8_t file_buf[FILE_BUF_SIZE];
	int bytes_read = 0;
	int block_count = 0;
	
	while ((bytes_read =
	  (int) fread(file_buf, 1, FILE_BUF_SIZE, p_bytecode_file)) > 0)	// size_t will be less tan or eq to 1024, so it's safe to cast to int
	{

		// TEST
		printf("bytes read: %d\n", bytes_read);
		for (int i = 0; i < bytes_read; i++)
			printf("HEX BYTE: %X\n", file_buf[i]);
		// TEST END

		block_count++;
		uint8_t *new_p_bytes =
		  realloc(p_bytes , (size_t) (FILE_BUF_SIZE * block_count));

		if (new_p_bytes == NULL)
		{
			fprintf(stderr, "Failed to realloc memory for bytecode instructions\n");
			exit(EXIT_FAILURE);
		}

		p_bytes = new_p_bytes;
		memcpy(p_bytes, file_buf, FILE_BUF_SIZE);
		printf("bytes cache size: %d\n", FILE_BUF_SIZE * block_count);
	}

	free(p_bytes);
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
