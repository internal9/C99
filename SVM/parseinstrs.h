#include <stdint.h>
#include <stdbool.h>

#ifndef PARSEINSTRSH
#define PARSEINSTRSH
enum InstrType {
        // One arg instructions
        PRINTREG,
        INC,
        DEC,
        PUSH,
        POP,

        // Two arg instructions
        MOV,
        LDA,
        ADD,
        SUB,
        MUL
};

enum Reg {
        R1,
        R2,
        R3,
        R4,
        RBP,
        RSP,
        RIP
};

enum ArgType {
        REG,
        INT8
};

struct Instr {
        enum InstrType type;
        enum ArgType arg1_type;
        enum ArgType arg2_type;
        union {
                enum Reg reg;
                int8_t int8;
        } arg1;
        union {
                enum Reg reg;
                int8_t int8;
        } arg2;
};

struct Instr *parse_src(char src[], int *instrs_len);
void ___debug_instr(struct Instr *instr);
#endif
