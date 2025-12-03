#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>

// Gonna put these parameterized macros into header files
// 'ERREXIT' & 'PERREXIT' expect *string literals* as the first argument format
#define ERREXIT(...) (fprintf(stderr, __VA_ARGS__), putc('\n', stderr), exit(EXIT_FAILURE))     // macro 'putc', yeah just don't have expressions wide side effects and you'lll be fine!!!!
#define PERREXIT(...) (fprintf(stderr, __VA_ARGS__), fputs(": ", stderr), perror(NULL), exit(errno))
#define TAB_WIDTH 8       // Assumption, despite ambiguity 

enum TkGroup {
	MISC
};

enum TkType {
        END,        // 'stdlib.h' defines EOF macro for file handling
        ARITH_OP,
	BOOL_OP,
        INT,
        NUM,
        KEYWORD,
        STRING,
        CHAR,
        MISC,
};

struct Tk {
        const char *type_str;
        size_t len;
        long line;      // ftell is archaic and returns a 'long'
        long column;
  	enum TkSubType
        enum TkType type;
};

static long src_line = 0;
static long src_column = 0;
static long src_i = 0;
static long src_len;
static char *src_txt;

// NOTE: Might have to change later if wanting variadic args
#define WARN(msg) printf("WARNING (L%ld C%ld): " msg "\n", src_line, src_column)
// NOTE: May remove macros and use a variable to just set both of these once per token lexed
#define INCPOS() (src_i++, src_column++)
#define ADDPOS(n) (src_i+=n, src_column+=n)


static void lex_int_or_num(struct Tk *p_tk, bool found_decimal)
{
        char c;
        p_tk->type = INT;
        while (true) {
                c = src_txt[src_i];
                if (c == '.' && !found_decimal) {
                        found_decimal = true;
                        p_tk->type = NUM;
                } else if (!isdigit(c))
                        break;
                INCPOS();
        }
}

static void set_tk(struct Tk *p_tk)
{
        if (src_i == src_len) {
                p_tk->type = END;
                p_tk->line = src_line;
                p_tk->column = src_column;
                return;
        }

        char c;
        while (isspace(c = src_txt[src_i++])) {
                if (c == '\t') {
                        WARN("Tab character '\t' width assumed to be 8 spaces despite ambiguity "
                                "width which may lead to inaccurate character column numbers in debugging");
                        ADDPOS(TAB_WIDTH);
                } else if (c == '\n') {
                        src_line++;
                        src_column = 0;
                } else
                        INCPOS();
        }

	p_tk->len = 1;
        p_tk->line = src_line;
        p_tk->column = src_column;
        
        switch (c) {
        case '+':
        case '-':
		p_tk->type = ARITH_OP;
                char peek_c = src_txt[src_i];
		
                if (peek_c == c)
			p_tk->len++, src_i++;
        case '/':       
        case '*':
		
                p_tk->type = ARITH_OP;
        case '-':
        case '-':
        case '-':
        case '-':
        case '-':
        case '-':
          
        }
        if (isdigit(c))
                lex_int_or_num(p_tk, false);
        else if (c == '.')
                lex_int_or_num(p_tk, true);
        else if (isalpha(c))
                lex_keyword_or_identifier(p_tk);
}

// should probably rework this to buffer instead of copying into a file
static void init_src_file(FILE *src_file)
{
        // fseek & ftell for portability to windows too ig???
        if (fseek(src_file, 0, SEEK_END) != 0)
                goto read_err;

        if ((src_len = ftell(src_file)) == -1L)
                goto read_err;

        if (fseek(src_file, 0, SEEK_SET) != 0)  // heard setting it to start is safe, i'm paranoid tho
                goto read_err;

        // I would use a VLA but I can't gracefully handle those errors if a stack overflow happens
        if ((src_txt = malloc((size_t) src_len)) == NULL)
                goto read_err;

        fread(src_txt, 1, (size_t) src_len, src_file);
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
        printf("Source file size: %ld\n", src_len);
}

// barebones for testing purposes
int main(int argc, const char *argv[])
{
        FILE *src_file = fopen(argv[2], "r");
        if (src_file == NULL)
                // ?: Maybe don't assume that errno is set?
                PERREXIT("Failed to open source file");

        init_src_file(src_file);
        //      gen_bytecode_file();
        return EXIT_SUCCESS;
}
