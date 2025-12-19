/*
  Option that doesn't exit compilation on error
*/

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>

// Gonna put these parameterized macros into header files
// 'ERREXIT' & 'PERREXIT' expect *string literals* as the first argument format
#define ERREXIT(...) (fprintf(stderr, __VA_ARGS__), putc('\n', stderr), exit(EXIT_FAILURE))     // macro 'putc', yeah just don't have expressions wide side effects and you'lll be fine!!!!
#define PERREXIT(...) (fprintf(stderr, __VA_ARGS__), fputs(": ", stderr), perror(NULL), exit(errno))
#define TAB_WIDTH 8       // Assumption, despite ambiguity 

enum tk_type {
        // assignment operators
        OP_AS,
        OP_ADD_AS,
        OP_SUB_AS,
        OP_MUL_AS,
        OP_DIV_AS,
        OP_POW_AS,
        OP_MOD_AS,
        OP_BSHL_AS,
        OP_BSHR_AS, 
        OP_BNOT_AS,
        OP_BOR_AS,
        OP_BAND_AS,
        OP_BXOR_AS,

        // arithmetic ops
        OP_ADD,
        OP_SUB,
        OP_INC,
        OP_DEC,
        OP_MUL,
        OP_DIV,
        OP_POW,
        OP_MOD,

        // logical ops
        OP_EQ,
        OP_NOT,
        OP_NOT_EQ,
        OP_LESS,
        OP_GREATER,
        OP_LESS_OR_EQ,
        OP_GREATER_OR_EQ,
        OP_AND,
        OP_OR,

        // bitwise ops
        OP_BSHL,        // shift left
        OP_BSHR,        // shift right
        OP_BAND,
        OP_BNOT,
        OP_BXOR,
        OP_BOR,

        // keyword
        KW_BOOL,
        KW_CHAR,
        KW_INT,
        KW_NUM,
        KW_STRING,
        KW_ARRAY,
        KW_STRUCT,
        KW_DYNAMIC,
        KW_CONST,
        KW_IF,
        KW_ELIF,
        KW_ELSE,
        KW_WHILE,
        KW_DO_WHILE,
        KW_FUNC,

        // literals
        LIT_BOOL,
        LIT_CHAR,
        LIT_INT,
        LIT_NUM,
        LIT_STR,

        // misc
        /*
          misc tokens can be interpreted in *various* ways,
          or are only ones serving *unique* purposes
        */
        IDENTIFIER,        
        END,
        PAREN_L,
        PAREN_R,
        BRACKET_L,
        BRACKET_R,
        BRACE_L,
        BRACE_R,
        PERIOD,
        COLON,
        SEMICOLON,
        QUESTION,
};

enum tk_type_group
{
        G_OP_ASSIGN,
        G_OP_ARITH,
        G_OP_LOGICAL,
        G_OP_BITWISE,
        G_KEYWORD,
        G_LITERAL,
        G_MISC
};

struct tk {
        // Add 'type_group' type_str for debugging?
        union {
                const char *txt;  // Used by 'LIT_STR' & 'IDENTIFIER'
                int64_t int_v;         // Used by 'LIT_INT'
                double fp_v;           // Used by 'LIT_FP'
                unsigned char c;                // Used by 'LIT_CHAR'
        } value;
        const char *type_str;
        long len;
        long line;      // ftell is archaic and returns a 'long', thus 'len' *also* has to be a 'long'
        long column;
        enum tk_type_group type_group;
        enum tk_type type;
};



static long src_line = 0;
static long src_column = 0;
static long src_i = 0;
static long src_len;
static unsigned char *src_txt;

// NOTE: Might have to change later if wanting variadic args
#define WARN(msg) printf("WARNING (L%ld C%ld): " msg "\n", src_line, src_column)
#define ERROR(msg) fprintf(stderr, "ERROR (L%ld C%ld): " msg "\n", src_line, src_column)

// NOTE: May remove macros and use a variable to just set both of these once per token lexed
#define INCPOS() (src_i++, src_column++)
#define ADDPOS(n) (src_i += n, src_column += n)
#define GET_C() src_txt[src_i]

// support octal integers?
static void lex_bin_int(struct tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_INT;
	
        unsigned char c = GET_C();	
        if (c != '0' || c != '1')
                ERROR("Expected digits after binary integer prefix.");

        do {
                if (!isdigit(c));
        } while (c);
}

static void lex_hex_int(struct tk *p_tk)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_INT;
        p_tk->value.int_v = 0;

        unsigned char c = GET_C();
        if (!isdigit(c))
                ERROR("Expected digits after hexadecimal integer literal prefix.");

        for (int i = 0; i < 8; i++) {
                INCPOS();
                switch (c = GET_C()) {
                case 'a': case 'A': p_tk->value.int_v <<= 4 | 10; break;
                case 'b': case 'B': p_tk->value.int_v <<= 4 | 11; break;
                case 'c': case 'C': p_tk->value.int_v <<= 4 | 12; break;
                case 'd': case 'D': p_tk->value.int_v <<= 4 | 13; break;
                case 'e': case 'E': p_tk->value.int_v <<= 4 | 14; break;
                case 'f': case 'F': p_tk->value.int_v <<= 4 | 15; break;
                default:
                        if (isdigit(c))
                                p_tk->value.int_v <<= 4 | (c - '0');
                        else
                                return;
                }
        }
                
        INCPOS();
        if (isxdigit(GET_C()))
                ERROR("Hexadecimal integer literal exceeds 8 digits, above 64-bit range");
}

static void lex_dec_int_or_num(struct tk *p_tk, bool found_decimal)
{
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_INT;

        unsigned char c;
        while (true) {
                c = GET_C();
                if (c == '.' && !found_decimal) {
                        found_decimal = true;
                        p_tk->type = LIT_NUM;
                        INCPOS();
                } else if (isdigit(c))
                        INCPOS();
                else
                        break;
        }
}

static void lex_op_other_or_assign(struct tk *p_tk, enum tk_type_group op_type_group,
                        enum tk_type op_type, enum tk_type assigner_type)
{
        if (GET_C() == '=') {
                p_tk->type_group = G_OP_ASSIGN;
                p_tk->type = assigner_type;
                INCPOS();
        } else {
                p_tk->type_group = op_type_group;
                p_tk->type = op_type;
        }
}

// will finish
/*
static void lex_keyword_or_identifier(struct tk *p_tk)
{
        unsigned char c = src_txt[src_i];
        while (isalpha(c) || isdigit(c) || c == '_') {
                INCPOS();
                c = GET_C();
        }
        // test this later
        // remove 'len' from struct? and just make it local?
        p_tk->len = src_column - p_tk->column;
        void *keyword_val = hashmap_get(p_tk, p_tk->data, p_tk->len);
        if (keyword_val != NULL) {
                p_tk->type;
        }
        
}
*/

// Only *ascii* chars will be supported, may be changed to *utf-8*, and allow wide chars
static void lex_literal_char(struct tk *p_tk)
{
        unsigned char c = GET_C();
        if (!isalpha(c) || c != '_')
                ERROR("Non-ASCII characters are unsupported.");

        INCPOS();
        if (GET_C() != '\'')
                ERROR("Expected ''' to end character literal.");

        INCPOS();
        p_tk->type_group = G_LITERAL;
        p_tk->type = LIT_CHAR;
        p_tk->value.c = c;
}

static void lex_literal_str(struct tk *p_tk)
{

}

static void lex_keyword_or_identifier(struct tk *p_tk)
{

}

static void handle_whitespace(void) {
        unsigned char c;
        while (isspace(c = GET_C())) {
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
}

static void handle_line_comment(void)
{
        while (GET_C() != '\n' || src_i != src_len);
}

static void handle_multi_line_comment(void)
{

}
// TODO: deal with 'src_i' & 'column'
static void lex_next(struct tk *p_tk)
{
        if (src_i == src_len) {
                p_tk->type_group = G_MISC;
                p_tk->type = END;
                p_tk->line = src_line;
                p_tk->column = src_column;
                return;
        }

        handle_whitespace();
        unsigned char c = GET_C();

        // next char after 'c'
        INCPOS();

        p_tk->len = 1;
        p_tk->line = src_line;
        p_tk->column = src_column;
        
        switch (c) {
        case '+':
                if (GET_C() == '+') {
                        INCPOS();
                        p_tk->type_group = G_OP_ARITH;
                        p_tk->type = OP_INC;
                } else
                        lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_SUB, OP_SUB_AS);
                break;
        case '-':
                if (GET_C() == '-') {
                        INCPOS();
                        p_tk->type_group = G_OP_ARITH;
                        p_tk->type = OP_DEC;
                } else
                        lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_SUB, OP_SUB_AS);
                break;
        case '/':
                if (GET_C() == '/')
                        handle_line_comment();
                else if (GET_C() == '*')
                        handle_multi_line_comment();
                else
                        lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_DIV, OP_DIV_AS);
                break;
        case '*':
                lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_MUL, OP_MUL_AS);
                break;
        case '!':
                if (GET_C() == '=') {
                        INCPOS();
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_BNOT_AS;
                } else {
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_BNOT;
                } 
                break;
        case '^':
                if (GET_C() == '^') {
                        INCPOS();
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BXOR, OP_BXOR_AS);
                } else
                        lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_POW, OP_POW_AS);
                break;
        case '%':
                lex_op_other_or_assign(p_tk, G_OP_ARITH, OP_MOD, OP_MOD_AS);
                break;
        case '&': {
                // maybe add incpos before siwtch statement?
                if (GET_C() == '&') {
                        INCPOS();
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_AND;
                } else
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BAND, OP_BAND);
                break;
        }
        case '|':
                if (GET_C() == '|') {
                        INCPOS();
                        p_tk->type_group = G_OP_LOGICAL;
                        p_tk->type = OP_OR;
                } else
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BOR, OP_BOR_AS);
                break;
        case '<':
                if (GET_C() == '<') {
                        INCPOS();
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BSHL, OP_BSHL_AS);
                } else {
                        if (GET_C() == '=') {
                                INCPOS();
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_LESS_OR_EQ;
                        } else { 
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_LESS;
                        }
                }
                break;
        case '>':
                if (GET_C() == '>') {
                        INCPOS();
                        lex_op_other_or_assign(p_tk, G_OP_BITWISE, OP_BSHR, OP_BSHR_AS);
                } else {
                        if (GET_C() == '=') {
                                INCPOS();
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_GREATER_OR_EQ;
                        } else { 
                                p_tk->type_group = G_OP_LOGICAL;
                                p_tk->type = OP_GREATER;
                        }
                }
                break;
        case '?':
                p_tk->type_group = G_MISC;
                p_tk->type = QUESTION;
                break;
        case ':':
                p_tk->type_group = G_MISC;
                p_tk->type = COLON;
                break;
        case '\'':        // 'LIT_CHAR'
                lex_literal_char(p_tk);
                break;
        case '"':        // 'LIT_STR'
                lex_literal_str(p_tk);
                break;
        case '0':
                if (GET_C() == 'x') {
                        INCPOS();
                        lex_hex_int(p_tk);
                } else if (GET_C() == 'b') {
                        INCPOS();
                        lex_bin_int(p_tk);
                } else
                        lex_dec_int_or_num(p_tk, false);
                break;
        case '.':
                if (isdigit(GET_C())) {
                        INCPOS();
                        lex_dec_int_or_num(p_tk, true);
                } else {
                        p_tk->type_group = G_MISC;
                        p_tk->type = PERIOD;
                }
                break;
        case '_':
                lex_keyword_or_identifier(p_tk);
                break;
        default:
                if (isdigit(c))
                        lex_dec_int_or_num(p_tk, false);
                else if (isalpha(c))
                        lex_keyword_or_identifier(p_tk);
        }
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
        if (argc != 2)
                ERREXIT("Expected one source file to compile to bytecode");

        FILE *src_file = fopen(argv[1], "r");
        if (src_file == NULL)
                // ?: Maybe don't assume that errno is set?
                PERREXIT("Failed to open source file");

        init_src_file(src_file);
        //      gen_bytecode_file();
        return EXIT_SUCCESS;
}
