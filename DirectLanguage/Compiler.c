#include <stdlib.h>
#include <stdio.h>
#include <errno.h>


// Gonna put these parameterized macros into header files
// 'ERREXIT' & 'PERREXIT' expect *string literals* as the first argument format
#define ERREXIT(...) (fprintf(stderr, __VA_ARGS__), putc('\n', stderr), exit(EXIT_FAILURE))	// macro 'putc', yeah just don't have expressions wide side effects and you'lll be fine!!!!
#define PERREXIT(...) (fprintf(stderr, __VA_ARGS__), fputs(": ", stderr), perror(NULL), exit(errno))

enum TkType {
	INTEGER,
	NUMBER,
	KEYWORD,
	STRING,
	CHAR,
	MISC,
};

struct Tk {
	const char *c;
	enum TkType type;
};

static long src_txt_i = 0;
static long src_txt_len;
static char *src_txt;

static void next_tk(void)
{
	
}


static void gen_bytecode_file(void)
{
	
}

// should probably rework this
static void init_src_file(FILE *src_file)
{
	// fseek & ftell for portability to windows too ig???
	if (fseek(src_file, 0, SEEK_END) != 0)
		goto read_err;

	if ((src_txt_len = ftell(src_file)) == -1L)
		goto read_err;

	if (fseek(src_file, 0, SEEK_SET) != 0)	// heard setting it to start is safe, i'm paranoid tho
		goto read_err;

	// I would use a VLA but I can't gracefully handle those errors if a stack overflow happens
	if ((src_txt = malloc((size_t) src_txt_len)) == NULL)
		goto read_err;

	fread(src_txt, 1, (size_t) src_txt_len, src_file);
	if (ferror(src_file))
		goto read_err;

	goto read_success;

read_err:
	free(src_txt);
	perror("Failed to read source file");

	if (fclose(src_file) != 0)
		perror("Failed to close source file");

	exit(EXIT_FAILURE);
	
read_success:
	if (fclose(src_file) != 0) {
		free(src_txt);
		perror("Failed to close source file");		
	}

	// debug
	printf("Source file size: %ld\n", src_txt_len);
}

// barebones for testing purposes
int main(int argc, const char *argv[])
{
	FILE *src_file = fopen(argv[2], "r");
	if (src_file == NULL)
		// ?: Maybe don't assume that errno is set?
		PERREXIT("Failed to open source file");

	init_src_file(src_file);
	gen_bytecode_file();
	return EXIT_SUCCESS;
}
