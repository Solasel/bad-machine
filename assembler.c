/*
   Takes a file with bal commdands and turns it into
   a file with equivalent bml instructions to be
   passed into logisim and be run.

   For all of you vim users, this file has a view file
   called assembler.c.v

   USE IT!
*/

/* Constants. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NUM_INSTRUCTIONS (27)
#define NUM_REGISTER (8)

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

/* Function Prototypes. */

/* Parses bal code in source into bml instructions in dest. */
int assemble_and_write(const char *src, const char *dst);

/* Gets a null-terminated list of the instructions
 * in src_text and writes it to inst_lst. */
int get_inst_lst(char *src_text, char ***inst_lst);

/* Writes the bml equivalent of the bal in linep
 * into dst, in hex. */
int line_to_inst(int i, char **linep, char **dst);

/* Writes inst_lst to dst in a format that logisim gets. */
int write_instuctions(**inst_lst, const char *dst);

/* Reads the contents of src and puts it into buf.
 * buf is double-null terminated. */
int read_file(const char *src, char **buf);

/* Frees a null-terminated list of strings. */
int free_str_lst(char **str_lst);

/* Formats instr as an R-style instruction
 * with given arg1, arg2, and arg3, and
 * S1 and S2. */
int write_r_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1, int s2);

/* Formats instr as an I-style instruction
 * with given arg1, arg2, and arg3, and S1. */
int write_i_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1);

/* Formats instr as an S-style instruction
 * with given arg1, arg2, and arg3. */
int write_s_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3);

/* Formats instr as an U-style instruction
 * with given arg1 and arg2. */
int write_u_type(int line, uint16_t *instr,
		char *arg1, char *arg2);

/* Sets the OPC field of instr to the given value. */
int set_opc(int opc, uint16_t *instr);

/* Sets the S1 field of instr to the given value. */
int set_s1(int s1, uint16_t *instr);

/* Sets the S2 field of instr to the given value. */
int set_s2(int s2, uint16_t *instr);

/* Makes sure that reg is a valid register. */
int is_invalid_reg(char *reg)

/* Given a valid register arg,
 * writes it to the rsX field of instr.
 * x < 0 ==> rs2
 * x = 0 ==> rs0
 * x > 0 ==> rs1	*/
int decode_rsx(int x, char *arg, uint16_t *instr);

/* Makes sure that imm is a valid decimal number. */
int is_invalid_imm(char *imm);

/* Parses imm as a signed value, and writes
 * it to I-type instr appropriately. */
int decode_i_type_imm(char *imm, uint16_t *instr);

/* Parses imm as a signed value, and writes
 * it to S-Type instr appropriately. */
int decode_s_type_imm(char *imm, uint16_t *instr);

/* Parses imm as a signed value, and writes
 * it to U-Type instr appropriately. */
int decode_u_type_imm(char *imm, uint16_t *instr);

/* Takes instr and converts it into a 4-digit
 * hex representation, stored in buf. */
int instr_to_hex(uint16_t instr, char **buf);

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


/*
 * #################
 * ### MAIN CODE ###
 * #################
*/


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
	int inst_count = 1;
	char *temp_src = strdup(src_text);

	if (!strtok(temp_src, "\r\n")) {
		printf("Empty source file.\n")
		failure = -1;
		goto free_temp_src;
	}

	while (strtok(NULL, "\r\n"))
		inst_count++;

	printf("%d instructions in source file.\n", inst_count);

	if (inst_count > 256) {
		printf("Too many instructions in source file.\n"
				"You can put a max of 256 instructions.\n");
		failure = -1;
		goto free_temp_src;
	}

	/* Initializes a null-terminated array
	 * to write instructions to. */
	*inst_lst = malloc((inst_count + 1) * sizeof(char *));
	if (!inst_lst) {
		printf("Failed to malloc a list of instructions at %d.\n",
				__LINE__);
		failure = 1;
		goto free_temp_src;
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
free_temp_src:
	free(temp_src);
	return failure

}


/* Writes the bml equivalent of the bal in linep
 * into dst, in hex. */
int line_to_inst(int i, char **linep, char **dst)
{
	int failure;
	uint16_t instr;

	char *arg1, *arg2, *arg3;

	char *line = *linep;
	char *token;

	/* Figures out whether the command is valid. */
	token = strtok(line, ", \t\v\f");
	for (i = 0; i < NUM_INSTRUCTIONS; i++) {
		if (!strcmp(token, instructions[i])) {
			break;
		}
	}

	if (i == num_instructions) {
		printf("invalid instruction: %s at line %d.\n", line, i);
		return -2;
	}

	/* Once we are sure we have an instruction,
	 * rip the arguments from it and check
	 * for trailing characters. */
	arg1 = strtok(NULL, ", \t\v\f");
	arg2 = strtok(NULL, ", \t\v\f");
	arg3 = strtok(NULL, ", \t\v\f");

	if (strtok(NULL, ", \t\v\f")) {
		printf("Trailing characters on line %d.\n",
				i);
		return -2;
	}
	

	/* Now that we know we have a valid instruction,
	 * check all the fields are correct, and formats
	 * instr properly. */
	switch(i) {

		/* r-types. */
		case 0 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					0, 0);
			break;

		case 1 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					0, 1);
			break;

		case 2 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					1, 0);
			break;

		case 3 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					1, 1);
			break;

		case 4 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					2, 0);
			break;

		case 5 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					3, 0);
			break;

		case 6 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					3, 1);
			break;

		case 7 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					4, 0);
			break;

		case 8 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					4, 1);
			break;

		case 9 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					5, 0);
			break;

		case 10 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					5, 1);
			break;

		case 11 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					6, 0);
			break;

		case 12 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					6, 1);
			break;

		case 13 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					7, 0);
			break;

		case 14 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					7, 1);
			break;

		case 15 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					7, 2);
			break;

		case 16 :
			failure = write_r_type(i, &instr,
					arg1, arg2, arg3,
					7, 3);
			break;

		/* I-Types. */
		case 17 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					0);
			break;

		case 18 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					1);
			break;

		case 19 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					2);
			break;

		case 20 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					3);
			break;

		case 21 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					4);
			break;

		case 22 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					5);
			break;

		case 23 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					6);
			break;

		case 24 :
			failure = write_i_type(i, &instr,
					arg1, arg2, arg3,
					7);
			break;

		/* S-Types. */
		case 25 :
			failure = write_s_type(i, &instr,
					arg1, arg2, arg3);
			break;

		/* U-Types. */
		case 26 :
			failure = write_u_type(i, &instr,
					arg1, arg2);
			break;

	}

	if (failure) {
		printf("Invalid instruction: %s at line %d.\n", line, i);
		return -2;
	}
	

	/* Finally, interprets instr and puts its
	 * hex representation into dst. */
	failure = inst_to_hex(instr, dst);
	if (failure) {
		printf("Failed to turn an instruction into hex at line %d.\n",
				__LINE__);
	}

	return failure;

}


/* Writes inst_lst to dst in a format that logisim gets. */
int write_instuctions(**inst_lst, const char *dst)
{
	int failure = 0;
	int i = 1;

	FILE *target;
	target = fopen(dst, "w");
	if (!target) {
		printf("Failed to create a bml file with name %s at %d.\n",
				dst, __LINE__);
		return 1;
	}

	fputs("v2.0 raw\n" target);

	while (*inst_lst) {
		fputs(*inst_lst, dst);

		if (i % 8 == 0)
			fputc('\n', dst);
		else
			fputc(' ', dst);
		inst_lst++;
		i++;
	}

	fclose(target);
	return failure;
}


/*
 * ###############
 * ### HELPERS ###
 * ###############
*/


/* Reads the contents of src and puts it into buf.
 * buf is double-null terminated. */
int read_file(const char *src, char **buf)
{
	int failure = 0;

	FILE *source;
	long len;

	source = fopen(dst, "rb");
	if (!source) {
		printf("Failed to read from file %s at %d.\n",
				src, __LINE__);
		return 1;
	}

	fseek(source, 0, SEEK_END);
	len = ftell(source);
	fseek(source, 0, SEEK_SET);

	*buf = malloc((len + 2) * sizeof(char));
	if (!*buf) {
		printf("Failed to malloc filestring for %s at %d.\n",
				src, __LINE__);
		failure = 1;
		goto close_file;
	}

	fread(*buf, 1, len, fp);
	(*buf)[len] = '\0';
	(*buf)[len + 1] = '\0';

close_file:
	fclose(source);
	return failure;


/* Frees a null-terminated list of strings. */
int free_str_lst(char **str_lst)
{
	char **temp = str_lst;
	while (*temp)
		free(*(temp++));
	free(str_lst);

	return 0;
}


/* Formats instr as an R-style instruction
 * with given arg1, arg2, and arg3, and
 * S1 and S2. */
int write_r_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1, int s2)
{

	set_opc(0, instr);
	set_s1(s1, instr);
	set_s2(s2, instr);

	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	} else
		decode_rsx(0, arg1, instr);

	if (is_invalid_reg(arg2)) {
		printf("Invalid register: %s at %d.\n",
				arg2, line);
		return -2;
	} else
		decode_rsx(1, arg2, instr);

	if (is_invalid_reg(arg3)) {
		printf("Invalid register: %s at %d.\n",
				arg3, line);
		return -2;
	} else
		decode_rsx(-1, arg3, instr);

	return 0;
}


/* Formats instr as an I-style instruction
 * with given arg1, arg2, and arg3, and S1. */
int write_i_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1)
{

	set_opc(1, instr);
	set_s1(s1, instr);

	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	} else
		decode_rsx(0, arg1, instr);

	if (is_invalid_reg(arg2)) {
		printf("Invalid register: %s at %d.\n",
				arg2, line);
		return -2;
	} else
		decode_rsx(1, arg2, instr);

	if (is_invalid_imm(arg3)) {
		printf("Invalid immediate: %s at %d.\n",
				arg3, line);
		return -2;
	} else
		decode_i_type_imm(arg3, instr);

	return 0;
}



/* Formats instr as an S-style instruction
 * with given arg1, arg2, and arg3. */
int write_s_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3)
{

	set_opc(2, instr);

	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	} else
		decode_rsx(-1, arg1, instr);

	if (is_invalid_reg(arg2)) {
		printf("Invalid register: %s at %d.\n",
				arg2, line);
		return -2;
	} else
		decode_rsx(1, arg2, instr);

	if (is_invalid_imm(arg3)) {
		printf("Invalid immediate: %s at %d.\n",
				arg3, line);
		return -2;
	} else
		decode_s_type_imm(arg3, instr);

	return 0;
}

/* Formats instr as an U-style instruction
 * with given arg1 and arg2. */
int write_u_type(int line, uint16_t *instr,
		char *arg1, char *arg2)
{

	set_opc(3, instr);

	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	} else
		decode_rsx(0, arg1, instr);

	if (is_invalid_imm(arg2)) {
		printf("Invalid immediate: %s at %d.\n",
				arg2, line);
		return -2;
	} else
		decode_u_type_imm(arg2, instr);

	return 0;
}

/* Sets the OPC field of instr to the given value. */
int set_opc(int opc, uint16_t *instr)
{
	set_bit(instr, 0, get_bit(opc, 0));
	set_bit(instr, 1, get_bit(opc, 1));
	return 0;
}


/* Sets the S1 field of instr to the given value. */
int set_s1(int s1, uint16_t *instr)
{
	set_bit(instr, 5, get_bit(s1, 0));
	set_bit(instr, 6, get_bit(s1, 1));
	set_bit(instr, 7, get_bit(s1, 2));
	return 0;
}


/* Sets the S2 field of instr to the given value. */
int set_s2(int s2, uint16_t *instr)
{
	set_bit(instr, 14, get_bit(s2, 0));
	set_bit(instr, 15, get_bit(s2, 1));
	return 0;
}


/* Makes sure that reg is a valid register. */
int is_invalid_reg(char *reg)
{
	int i;

	for (i = 0; i < NUM_REGISTERS; i++) {
		if (!strcmp(line, registers[i]))
			return 0;
	}

	return -2;
}


/* Given a valid register arg,
 * writes it to the rsX field of instr.
 * x < 0 ==> rs2
 * x = 0 ==> rs0
 * x > 0 ==> rs1	*/
int decode_rsx(int x, char *arg, uint16_t *instr)
{
	int i;

	for (i = 0; i < NUM_REGISTERS; i++) {
		if (!strcmp(line, registers[i]))
			break;
	}


	if (x < 0) {
		set_bit(instr, 11, get_bit(i, 0));
		set_bit(instr, 12, get_bit(i, 1));
		set_bit(instr, 13, get_bit(i, 2));
	} else if (x > 0) {
		set_bit(instr, 8, get_bit(i, 0));
		set_bit(instr, 9, get_bit(i, 1));
		set_bit(instr, 10, get_bit(i, 2));
	} else {
		set_bit(instr, 2, get_bit(i, 0));
		set_bit(instr, 3, get_bit(i, 1));
		set_bit(instr, 4, get_bit(i, 2));
	}

	return 0;

}


/* Makes sure that imm is a valid decimal number. */
int is_invalid_imm(char *imm)
{
	int i;
	for (i = 0; i < strlen(imm); i++) {
		if (imm[i] < '0' || imm[i] > '9')
			return 1;
	}
	return 0;
}


/* Parses imm as a signed value, and writes
 * it to I-type instr appropriately. */
int decode_i_type_imm(char *imm, uint16_t *instr)
{
	long imm_l = strtol(imm, NULL, 10);
	set_bit(instr, 11, get_bit(imm_l, 0));
	set_bit(instr, 12, get_bit(imm_l, 1));
	set_bit(instr, 13, get_bit(imm_l, 2));
	set_bit(instr, 14, get_bit(imm_l, 3));
	set_bit(instr, 15, get_bit(imm_l, 4));
	return 0;

}


/* Parses imm as a signed value, and writes
 * it to S-Type instr appropriately. */
int decode_s_type_imm(char *imm, uint16_t *instr)
{
	long imm_l = strtol(imm, NULL, 10);
	set_bit(instr, 5, get_bit(imm_l, 0));
	set_bit(instr, 6, get_bit(imm_l, 1));
	set_bit(instr, 7, get_bit(imm_l, 2));
	set_bit(instr, 14, get_bit(imm_l, 3));
	set_bit(instr, 15, get_bit(imm_l, 4));
	return 0;
}


/* Parses imm as a signed value, and writes
 * it to U-Type instr appropriately. */
int decode_u_type_imm(char *imm, uint16_t *instr)
{
	long imm_l = strtol(imm, NULL, 10);
	set_bit(instr, 5, get_bit(imm_l, 0));
	set_bit(instr, 6, get_bit(imm_l, 1));
	set_bit(instr, 7, get_bit(imm_l, 2));
	set_bit(instr, 8, get_bit(imm_l, 3));
	set_bit(instr, 9, get_bit(imm_l, 4));
	set_bit(instr, 10, get_bit(imm_l, 5));
	set_bit(instr, 11, get_bit(imm_l, 6));
	set_bit(instr, 12, get_bit(imm_l, 7));
	set_bit(instr, 13, get_bit(imm_l, 8));
	set_bit(instr, 14, get_bit(imm_l, 9));
	set_bit(instr, 15, get_bit(imm_l, 10));
	return 0;
}


/* Takes instr and converts it into a 4-digit
 * hex representation, stored in buf. */
int instr_to_hex(uint16_t instr, char **buf);
{
	*buf = malloc(5 * sizeof(char));
	if (!*buf) {
		printf("Failed to malloc space for an instruction at %d.\n",
				__LINE__);
		return 1;
	}

	sprintf(*buf, "%.4x", instr);

	return 0;

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

