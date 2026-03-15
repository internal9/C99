// init lexing here instead of in main.c?
#include "lex.h"
#include <stdio.h>
#include <stdint.h>

uint8_t *bytecode_gen_nofile(void)
{
        // 'main.c' initialized lexer, maybe change that cuz a lil confusing
        struct Tk tk;
        lex_next(&tk);        

        // debug
        printf("TOKEN TYPE: %d\n", tk.type);
}

FILE *bytecode_gen_file(void)
{
        
}
