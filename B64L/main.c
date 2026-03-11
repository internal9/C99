/*
  NOTES:
  - '-c' Only compile src and output bytecode file, no execution
  - '-n' Compile & run src, but *no* bytecode file output
  - have a seperate VM binary or just arg option to either run or just bytecode compile?
*/
#include "lex.h"
// #include "bytecode_gen.h"
// #include "VM.h"
#include "util.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ONLY_COMPILE "-c"
#define ONLY_RUN "-n"

// barebones for testing purposes
int main(int argc, const char *argv[])
{
        if (argc < 2)
                ERREXIT("Expected one source file to compile to bytecode,"
                        " or one bytecode file to execute.");
        if (argc > 3)
                ERREXIT("Excess arguments, expected optional flag and/or"
                        " one source file to compile to bytecode,"
                        " or one bytecode file to execute.");

        const char *file_name = argv[argc - 1];
        if (argc == 2) {
                lex_init(file_name);
                // vm_run(gen_bytecode());
        }
        else if (argc == 3) {
                const char *flag = argv[1];
                if (strcmp(flag, ONLY_COMPILE)) {
                        lex_init(file_name);
                        // bytes = bytecode_gen_file();
                }
                else if (strcmp(flag, ONLY_RUN)) {
                        lex_init(file_name);
                        // bytes = bytecode_gen_nofile();
                        // vm_run(bytes);
                }
                else
                        ERREXIT("Invalid flag '%s'\n", flag);
        }

        return EXIT_SUCCESS;
}
