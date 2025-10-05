#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "parseinstrs.h"

int main(void)
{
	char src[] =
	"MOV R1 R2;"
	"MOV R3 R4;";
	int instrs_len;
	struct Instr *parsed_instrs = parse_src(src, &instrs_len);

	printf("___DEBUG INSTRUCTIONS:\n");
	for (int i  = 0; i < instrs_len; i++)
		___debug_instr(parsed_instrs + i);
	return 0;
}
