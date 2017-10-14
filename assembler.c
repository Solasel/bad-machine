/*
   Takes a file with bal commdands and turns it into
   a file with equivalent bml instructions to be
   passed into logisim and be run.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NUM_INSTRUCTIONS (27)

const char *instructions[] = {
	"add", "sub",
	"slt", "sltu",
	"sll",
	"sra", "srl",
	"or", "nor",
	"and", "nand",
	"xor", "xnor",
	"mul", "mulh", "div", "rem",

	"addi", "slti", "slli", "srai",
	"ori", "andi", "xori", "lw",

	"sw",

	"lui"
};

const char *registers[] = {
	"x0", "x1", "x2", "x3",
	"x4", "x5", "x6", "x7"
};

/* Parses bal code in source into bml instructions in dest. */
int assemble_and_write(const char *src, const char *dst);

/* Gets a null-terminated list of the instructions
 * in src_text and writes it to inst_lst. */
int get_inst_lst(char *src_text, char ***inst_lst);

/* Converts line into a hex instruction
 * and stores that in dst. */
/* TODO */
int line_to_inst(int i, char **line, char **dst);

/* Writes inst_lst to dst in a format that logisim gets. */
/* TODO */
int write_instuctions(**inst_lst, const char *dst);

/* Reads the contents of src and puts it into buf.
 * buf is double-null terminated. */
/* TODO */
int read_file(const char *src, char **buf);

/* Frees a null-terminated list of strings. */
/* TODO */
int free_str_lst(char **str_lst);

/* Determines if arg is a valid register, and if so,
 * writes it to the rsX field of instr. */
/* TODO */
int decode_rsX(char *arg, uint16_t *instr);

/* Parses imm as a signed value, and writes
 * it to I-type instr appropriately. */
/* TODO */
int decode_i_type_imm(char *imm, uint16_t *instr);

/* Parses imm as a signed value, and writes
 * it to S-Type instr appropriately. */
/* TODO */
int decode_s_type_imm(char *imm, uint16_t *instr);

/* Parses imm as a signed value, and writes
 * it to U-Type instr appropriately. */
/* TODO */
int decode_u_type_imm(char *imm, uint16_t *instr);

/* Takes instr and converts it into a 4-digit
 * hex representation for writing. */
/* TODO */
char *instr_to_hex(uint16_t instr);

/* Gets the nth bit of x. */
unsigned get_bit(unsigned x, unsigned n);

/* Sets the nth bit of x to v. */
void set_bit(unsigned * x, unsigned n, unsigned v);

/* Main. */
int main(int argc, char *argv[])
{
	int failure; 

	/* Makes sure we have the adequate set of arguments. */
	if (argc != 3) { 
		printf("This assembler should take two arguments:\n\n");
		printf("./{exec} {source file(.bal)} {target file(.bml)}\n");

		failure = 1;
		goto end;
	}

	/* Attempts to parse argv[1] -> argv[2]. */
	printf("Attempting to parse %s into machine language -> %s...",
			argv[1], argv[2]); 

	failure = assemble_and_write(argv[1], argv[2]);

end:

	/* Handles errors and tells the user what happened. */
	if (failure)
		printf("Parsing failed, neither file has been changed.\n"
		"Error code: %d\n", failure);

	else 
		printf("Parsing successful!\n"
		"%s now contains bad machine langauge!\n", argv[2]);

	return failure;
}

/* Parses bal code in source into bml instructions in dest. */
int assemble_and_write(const char *src, const char *dst)
{
	int failure;

	char *src_buf;
	char **inst_lst;

	/* Reads the contents of src into src_buf (double null terminated). */
	failure = read_file(src, &src_buf);
	if (failure) {
		printf("Failed to read %s at %d.\n",
				src, __LINE__);

		return failure;
	}

	/* Loads the hex instructions into inst_lst. */
	failure = get_inst_lst(src_buf, &inst_lst);
	if (failure) {
		printf("Failed to get a list of instructions at %d.\n",
				__LINE__);
		goto free_src_buf;
	}

	/* Writes the list of instructions to dst. */
	failure = write_instuctions(inst_lst, dst);
	if (failure)
		printf("Failed to write instructions to %s at %d.\n",
				dst, __LINE__);


	/* Free things. */
	free_str_lst(inst_lst);
free_src_buf:
	free(src_buf);
	return failure;
}


/* Gets a null-terminated list of the instructions
 * in src_text and writes it to buf. */
int get_inst_lst(char *src_text, char ***inst_lst)
{
	int failure;
	int i;
	char *line;

	/* Gets the number of instructions. */
	int inst_count = 0;

	i = 0;
	while (src_text[i]) {
		if (src_text[i] == '\n')
			inst_count++;
		i++;
	}

	/* Initializes a null-terminated array
	 * to write instructions to. */
	*inst_lst = malloc((inst_count + 1) * sizeof(char *));
	if (!inst_lst) {
		printf("Failed to malloc a list of instructions at %d.\n",
				__LINE__);
		return 1;
	}

	*inst_lst[inst_count] = NULL;

	/* For each line in src_text, writes
	 * its instruction to inst_lst. */
	i = 0;
	while ((line = strtok(src_text, "\r\n"))) {

		failure = line_to_inst(i, &line, *inst_lst + i);
		if (failure) {
			printf("Failed to change a line to an instruction at "
					"%d.\n", __LINE__);
			goto free_inst_lst;
		}

		src_text = strchr(line, '\0') + 1;

		i++;
	}

free_inst_lst:
	free_str_lst(inst_lst);
	return failure

}

/* Writes the bml equivalent of the bal in linep
 * into dst, in hex. */
/* TODO */
int line_to_inst(int i, char **linep, char **dst)
{
	uint16_t i;

	int instr = 0;
	char *line = *linep;
	char *token;

	/* Writes OPC, S1, and S2. */
	token = strtok(line, " \t\v\f");
	for (i = 0; i < NUM_INSTRUCTIONS; i++) {

		if (!strcmp(line, instructions[i])) {
			
			switch(i) {

			}

			break;

		}
	}
	
	if (i == NUM_INSTRUCTIONS) {
		printf("Invalid instruction: %s at line %d.\n", line, i);
		return 1;
	}


}

// Return the nth bit of x.
// Assume 0 <= n <= 31
unsigned get_bit(unsigned x, unsigned n)
{
    // YOUR CODE HERE
    // Returning -1 is a placeholder (it makes
    // no sense, because get_bit only returns 
    // 0 or 1)
	return (x >> n) & 1;
}
// Set the nth bit of the value of x to v.
// Assume 0 <= n <= 31, and v is 0 or 1
void set_bit(unsigned * x, unsigned n, unsigned v)
{
	*x = (*x & ~(1 << n)) | (v << n);
}

