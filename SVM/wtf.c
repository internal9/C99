struct Instr *parse_src(char src[], int *instrs_len)
{
        struct Instr *parsed_instrs = NULL;
        srcp_cur = srcp_start = src; // only using a pointer to a local variable during it's lifetime, i think it's fine
        *instrs_len = 0;

        while(*srcp_cur != '\0') {      // do not enter a string initialized via an array without a null terminator at the end lol
                struct Instr instr;
                while (is_whitespace(*srcp_cur))
                        srcp_cur++;
                if (*srcp_cur == '\0') break;

                int instr_name_len = 0;
                char *instr_name = get_wordp(&instr_name_len);

                if (instr_name_len == 0) {
                        ERREXIT("Expected instruction at pos %d\n",
                          (int) (srcp_cur - srcp_start));
                }

                int statement_start_pos = (int) (instr_name - srcp_start);
                bool has_two_args = false;

                set_instr_type_info(&instr, instr_name,
                  instr_name_len, &has_two_args);

                int arg1_len;
                char *arg1 = get_wordp(&arg1_len);
                bool arg1_is_reg_only = (instr.type == PUSH) ? false : true;

                if (has_two_args) {
                        int arg2_len;
                        char *arg2 = get_wordp(&arg2_len);

                        if (arg1_len == 0) {
                                ERREXIT(
                                  "Expected 2 args for instruction starting at pos %d\n",
                                  statement_start_pos);
                        }

                        if (arg2_len == 0) {
                                ERREXIT(
                                  "Expected arg 2 for instruction starting at pos %d\n",
                                  statement_start_pos);
                        }

                        set_instr_arg_info(&instr, 1, arg1,
                          arg1_len, arg1_is_reg_only);
                        set_instr_arg_info(&instr, 2, arg2,
                          arg2_len, false);
                }
                else {
                        if (arg1_len == 0) {
                                ERREXIT(
                                  "Expected arg for instruction starting at pos %d\n",
                                  statement_start_pos);
                        }
                        set_instr_arg_info(&instr, 1, arg1,
                          arg1_len, arg1_is_reg_only);
                }

                (*instrs_len)++;

                // better than trying to manually free and malloc again
//              parsed_instrs = realloc(parsed_instrs,
//                (size_t) (*instrs_len * sizeof(struct Instr)));

                if (parsed_instrs == NULL) {
                        free(parsed_instrs);
                        ERREXIT("Failed to alloc memory for instructions\n");
                }

                parsed_instrs[*instrs_len] = instr;
                end_statement();
        }

        for (int i = 0; i < *instrs_len; i++)
                ___debug_instr(parsed_instrs + i);
        return parsed_instrs;
} 
