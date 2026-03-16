// lex literals (e.g. LIT_BOOL)
// init lexing here instead of in main.c?
#include "lex.h"
#include "VM.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static struct Tk tk;

static void
expr();

static void
decl(enum TkType value_type)
{
        uint8_t *code;
        if (lex_next(&tk) != IDENTIFIER) {
                // error
                printf("no\n");
                exit(1);
        }

        if (lex_next(&tk) == OP_AS) {
                // expr();
                // allow int to be assigned to char?
                if (lex_next(&tk) != value_type) {
                        printf("noo: %d %d\n", value_type, tk.type);
                        exit(1);
                }

                // '4' for int, just testing int rn
                code = malloc(1 + 4); // expr_size
                // push_decl()
                code[0] = PUSH; // handle op modes e.g. 'USE_REG'
                memcpy(&code[1], &tk.value.int_v, 4);

                printf("OPCODE %.2x: ARG: %.2x %.2x %.2x %.2x\n",
                       code[0], code[1], code[2], code[3],
                       code[4]);
        }

}

uint8_t *
bytecode_gen_nofile(void)
{
        // 'main.c' initialized lexer, maybe change that cuz a lil confusing
        do {
                lex_next(&tk);
                // debug
                printf("TOKEN TYPE: %d\n", tk.type);

                switch(tk.type) {
                        /* case KW_DYNAMIC: how to handle this then?
                        (e.g. dynamic int) */
                case KW_BOOL: 
                case KW_CHAR:
                case KW_INT: decl(LIT_INT); break;
                case KW_NUM:
                case KW_STRING:
                case KW_ARRAY:
                case KW_STRUCT:
                }
        } while (tk.type != END);
}

FILE *
bytecode_gen_file(void)
{
        
}
