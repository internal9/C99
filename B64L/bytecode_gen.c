// lex literals (e.g. LIT_BOOL)
// init lexing here instead of in main.c?
#include "lex.h"
#include "VM.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define tk_err(tk, MSG) fprintf(stderr, "(L%ld C%ld)" MSG, tk.line, tk.column)
#define tk_err_fmt(tk, FMT, ...) fprintf(stderr, "(L%ld C%ld)" MSG, \
                                         tk.line, tk.column, __VA_ARGS__)
// buffer tb for multiple tks? incase of backtracking, etc
static struct Tk tk;


uint8_t *gen_from_ident(struct Tk *p_tk, bool allow_standalone)
{
        switch (lex_next(&tk)) {
        case PAREN_L:
        case BRACKET_L:
        case PERIOD:
        case INC:
        case DEC:
        default:
                if (!allow_standalone)
                        ; // error(id cannot be by itself);
        }
}

static prefix_expr(struct Tk *p_tk)
{
        switch (p_tk->type) {
                // case G_LITERAL:
                // return gen_literal(p_tk)
        case LIT_BOOL:
        case LIT_CHAR:
        case LIT_INT:
        case LIT_NUM:
        case OP_SUB:// unary arith negate
        case OP_NOT: // logical negate
        case IDENTIFIER:
                return gen_from_ident(p_tk); 
        }
}


// unnamed aggregate objects allowed (e.g. func((int) {'a', 2, 5}); func({.asd = 20})
static void
expr(int prec_limit)
{
        struct Tk left;
        lex_next(&left);
        uint8_t *c = prefix_expr(&left);

        // operator
        while (prec( lex_next(&tk) ) > prec_limit) {
                int p = prec(tk.type);
                //                if (p == -1) break;
//                if (p <= prec_limit) break;
                // tern expr?
                c = join(c, gen_expr(p));
        }
}

// account for floating-points later, cuz fp regs

#define R1 uhh smthn

// recursion
// switch to a 'pratt-parsing' style for cleaner recursion?
// mix of global and local tks, what a mess!
static void
expr(struct Tk *p_tk)
{
        // make no longer static
        static int precs[] = {[OP_ADD] = 0, [OP_MUL] = 1};
        static struct Tk buf[512];
        static int i = 0;
        static int R1_pushes = 0;
        static bool R1_use = false;

        int s_i = i;
        int last_op_i = -1;
        for (;; lex_next(p_tk = (buf + i))) {
                switch (p_tk->type) {
                case PAREN_L: expr(); // intended fall through
                case IDENTIFIER:
                case LIT_INT:
                case LIT_NUM:
                        // get op or none
                        lex_next(p_tk = (buf + i++));
                        if (p_tk->type_group != wat)// op groups
                                break;
                        if (last_op_i != -1 && precs[p_tk->type] <= precs[(buf + last_op_i)->type]) {
                                struct Tk *l = buf + (last_op_i - 1);
                                struct Tk *r = buf + (last_op_i + 1);
                                /* process right first to check if right
                                   operand is a sub expr that used reg R1 */
                                if (r->type == INT)
                                        emit2(MOV, R2, r->value.int_v);
                                else { // R1
                                        emit2(MOV, R2, R1); // assume R1 was used for sub expr instrs
                                        R1_use = false;
                                }

                                if (l->type == INT) {
                                        if (R1_use == true) {
                                                emit1(PSH, R1);
                                                R1_pushes++;
                                        }
                                        emit2(MOV, R1, r->value.int_v);
                                }
                                else // R1
                                        if (R1_use != true) {
                                                emit(POP, R1);
                                                /* Assume another sub expr with higher
                                                   prec pushed this R1 to be used later */
                                                R1_pushes--;
                                        }
                                /* else assume reg R1 was last used *without* being pushed due sub-expr from *right* operand,
                                   no need to emit any instrs, just operate on R1 */
                                buf[last_op_i - 1] = R1; // use same 'last_op_i' replace compiled expr, other tks are now 'garbage'
                                i = last_op_i;
                        }
                        last_op_i = i;
                        break;
                default:
                }
                        
        }

        i = s_i; // in recursive calls, simply leave succeeding values as 'garbage'
}

static void
decl(enum TkType value_type)
{
        uint8_t *code;
        if (lex_next(&tk) != IDENTIFIER) {
                // error
                printf("no\n");
                exit(1);
        }

        char *id = tk.txt;
        if (lex_next(&tk) != OP_AS) {
                var(id
                return 1;
        }
        // expr();
        // allow int to be assigned to char?
        if (lex_next(&tk) != value_type) {
                tk_err_fmt("Expected '=' instead of ");
                                  
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

uint8_t *
bytecode_gen_nofile(void)
{
        // 'main.c' initialized lexer, maybe change that cuz a lil confusing
        do {
                // let gen do it instead
                //                lex_next(&tk);
                // debug
                printf("TOKEN TYPE: %d\n", tk.type);

                switch(tk.type) {
                        /* case KW_DYNAMIC: how to handle this then?
                        (e.g. dynamic int) */
                        // case TYPE:
                case KW_BOOL: // decl_bool;
                case KW_CHAR: // decl_char
                case KW_INT: // decl_int
                case KW_NUM: // decl_num
                case KW_STRING: // decl_str
                case KW_ARRAY:
                case KW_STRUCT:

                        // TYPE_MODIFIER
                        // case DYNAMIC:
                        // case CONST:

                        
                case FN:
                case KW_IF:
                        case ELIEF
                        case ELSE
                        case WHILE
                        case JMP
                }
        } while (tk.type != END);
}

FILE *
bytecode_gen_file(void)
{
        
}
