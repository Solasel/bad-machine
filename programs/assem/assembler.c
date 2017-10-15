/* Takes a file with bal commdands and turns it into
   a file with equivalent bml instructions to be
   passed into logisim and be run.

   For all of you vim users, this file has a view file
   called .assembler.c.v

   USE IT!						*/

/* Constants. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define NUM_INSTRUCTIONS (27)
#define NUM_REGISTERS (8)

static const char *instructions[] = {
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

static const char *registers[] = {
	"x0", "x1", "x2", "x3",
	"x4", "x5", "x6", "x7"
};

/* Function Prototypes. */

/* Parses bal code in source into bml instructions in dest. */
static int assemble_and_write(const char *src, const char *dst);

/* Gets a null-terminated list of the instructions
 * in src_text and writes it to inst_lst. */
static int get_inst_lst(char *src_text, char ***inst_lst);

/* Writes the bml equivalent of the bal in linep
 * into dst, in hex. */
static int line_to_inst(int line_num, char **linep, char **dst);

/* Writes inst_lst to dst in a format that logisim gets. */
static int write_instuctions(char **inst_lst, const char *dst);

/* Reads the contents of src and puts it into buf.
 * buf is double-null terminated. */
static int read_file(const char *src, char **buf);

/* Frees a null-terminated list of strings. */
static void free_str_lst(char **str_lst);

/* Formats instr as an R-style instruction
 * with given arg1, arg2, and arg3, and
 * S1 and S2. */
static int write_r_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1, int s2);

/* Formats instr as an I-style instruction
 * with given arg1, arg2, and arg3, and S1. */
static int write_i_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1);

/* Formats instr as an S-style instruction
 * with given arg1, arg2, and arg3. */
static int write_s_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3);

/* Formats instr as an U-style instruction
 * with given arg1 and arg2. */
static int write_u_type(int line, uint16_t *instr,
		char *arg1, char *arg2);

/* Sets the OPC field of instr to the given value. */
static void set_opc(int opc, uint16_t *instr);

/* Sets the S1 field of instr to the given value. */
static void set_s1(int s1, uint16_t *instr);

/* Sets the S2 field of instr to the given value. */
static void set_s2(int s2, uint16_t *instr);

/* Makes sure that reg is a valid register. */
static int is_invalid_reg(char *reg);

/* Given a valid register arg,
 * writes it to the rsX field of instr.
 * x < 0 ==> rs2
 * x = 0 ==> rs0
 * x > 0 ==> rs1	*/
static void decode_rsx(int x, char *arg, uint16_t *instr);

/* Makes sure that imm is a valid decimal number. */
static int is_invalid_imm(char *imm);

/* Parses imm as a signed value, and writes
 * it to I-type instr appropriately. */
static void decode_i_type_imm(char *imm, uint16_t *instr);

/* Parses imm as a signed value, and writes
 * it to S-Type instr appropriately. */
static void decode_s_type_imm(char *imm, uint16_t *instr);

/* Parses imm as a signed value, and writes
 * it to U-Type instr appropriately. */
static void decode_u_type_imm(char *imm, uint16_t *instr);

/* Takes instr and converts it into a 4-digit
 * hex representation, stored in buf. */
static int instr_to_hex(uint16_t instr, char **buf);

/* Gets the nth bit of x. */
static unsigned get_bit(uint16_t x, unsigned n);

/* Sets the nth bit of x to v. */
static void set_bit(uint16_t * x, unsigned n, unsigned v);

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
	printf("Attempting to parse %s into machine language -> %s...\n",
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

/* #################
 * ### MAIN CODE ###
 * ################# */

/* Parses bal code in source into bml instructions in dest. */
static int assemble_and_write(const char *src, const char *dst)
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
static int get_inst_lst(char *src_text, char ***inst_lst)
{
	int failure, i;
	char *line;

	/* Gets the number of instructions. */
	int inst_count = 1;
	char *temp_src = strdup(src_text);
	if (!temp_src) {
		printf("Failed to malloc a copy of source file at line %d.\n",
				__LINE__);
		return 1;
	}

	if (!strtok(temp_src, "\r\n")) {
		printf("Empty source file.\n");
		free(temp_src);
		return -1;
	}

	while (strtok(NULL, "\r\n"))
		inst_count++;
	free(temp_src);
	printf("%d instructions in source file.\n", inst_count);

	if (inst_count > 256) {
		printf("Too many instructions in source file.\n"
				"You can put a max of 256 instructions.\n");
		return -1;
	}

	/* Initializes a null-terminated array
	 * to write instructions to. */
	*inst_lst = malloc((inst_count + 1) * sizeof(char *));
	if (!*inst_lst) {
		printf("Failed to malloc a list of instructions at %d.\n",
				__LINE__);
		return 1;
	}

	(*inst_lst)[inst_count] = NULL;

	/* For each line in src_text, writes
	 * its instruction to inst_lst. */
	i = 0;
	while ((line = strtok(src_text, "\r\n"))) {
		failure = line_to_inst(i, &line, *inst_lst + i);
		if (failure) {
			printf("Failed to change a line to an instruction at "
					"%d.\n", __LINE__);
			free_str_lst(*inst_lst);
			return failure;
		}

		src_text = strchr(line, '\0') + 1;
		i++;
	}

	return failure;
}

/* Writes the bml equivalent of the bal in linep
 * into dst, in hex. */
static int line_to_inst(int line_num, char **linep, char **dst)
{
	char *delims = ", \t\v\f";
	int failure, i;
	uint16_t instr;
	char *arg1, *arg2, *arg3;
	char *token;

	/* Figures out whether the command is valid. */
	token = strtok(*linep, delims);
	for (i = 0; i < NUM_INSTRUCTIONS; i++) {
		if (!strcmp(token, instructions[i]))
			break;
	}

	if (i == NUM_INSTRUCTIONS) {
		printf("invalid instruction: %s at line %d.\n", *linep, line_num);
		return -2;
	}

	/* Once we are sure we have an instruction,
	 * rip the arguments from it and check
	 * for trailing characters. */
	arg1 = strtok(NULL, delims);
	arg2 = strtok(NULL, delims);
	arg3 = strtok(NULL, delims);

	*linep = arg3;
	if (strtok(NULL, delims)) {
		printf("Trailing characters on line %d.\n",
				line_num);
		return -2;
	}
	
	/* Now that we know we have a valid instruction,
	 * check all the fields are correct, and formats
	 * instr properly. */
	switch (i) {
	/* R-Types. */
	case 0:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 0, 0);
		break;
	case 1:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 0, 1);
		break;
	case 2:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 1, 0);
		break;
	case 3:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 1, 1);
		break;
	case 4:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 2, 0);
		break;
	case 5:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 3, 0);
		break;
	case 6:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 3, 1);
		break;
	case 7:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 4, 0);
		break;
	case 8:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 4, 1);
		break;
	case 9:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 5, 0);
		break;
	case 10:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 5, 1);
		break;
	case 11:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 6, 0);
		break;
	case 12:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 6, 1);
		break;
	case 13:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 7, 0);
		break;
	case 14:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 7, 1);
		break;
	case 15:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 7, 2);
		break;
	case 16:
		failure = write_r_type(line_num, &instr, arg1, arg2, arg3, 7, 3);
		break;
	/* I-Types. */
	case 17:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 0);
		break;
	case 18:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 1);
		break;
	case 19:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 2);
		break;
	case 20:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 3);
		break;
	case 21:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 4);
		break;
	case 22:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 5);
		break;
	case 23:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 6);
		break;
	case 24:
		failure = write_i_type(line_num, &instr, arg1, arg2, arg3, 7);
		break;
	/* S-Types. */
	case 25:
		failure = write_s_type(line_num, &instr, arg1, arg2, arg3);
		break;
	/* U-Types. */
	case 26:
		failure = write_u_type(line_num, &instr, arg1, arg2);
		break;
	}

	if (failure) {
		printf("Invalid instruction: %s at line %d.\n", *linep, line_num);
		return -2;
	}

	/* Finally, interprets instr and puts its
	 * hex representation into dst. */
	failure = instr_to_hex(instr, dst);
	if (failure)
		printf("Failed to turn an instruction into hex at line %d.\n",
				__LINE__);

	return failure;
}

/* Writes inst_lst to dst in a format that logisim gets. */
static int write_instuctions(char **inst_lst, const char *dst)
{
	int i;

	FILE *target;
	target = fopen(dst, "w");
	if (!target) {
		printf("Failed to create a bml file with name %s at %d.\n",
				dst, __LINE__);
		return 1;
	}

	fputs("v2.0 raw\n", target);
	for (i = 1; *inst_lst; i++, inst_lst++) {
		fputs(*inst_lst, target);
		fputc(i % 8 ? ' ': '\n', target);
	}

	fclose(target);
	return 0;
}

/* ###############
 * ### HELPERS ###
 * ############### */

/* Reads the contents of src and puts it into buf.
 * buf is double-null terminated. */
static int read_file(const char *src, char **buf)
{
	int failure = 0;
	FILE *source;
	long len;

	source = fopen(src, "rb");
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

	fread(*buf, 1, len, source);
	(*buf)[len] = '\0';
	(*buf)[len + 1] = '\0';

close_file:
	fclose(source);
	return failure;
}

/* Frees a null-terminated list of strings. */
static void free_str_lst(char **str_lst)
{
	char **temp = str_lst;
	while (*temp)
		free(*(temp++));
	free(str_lst);
}

/* Formats instr as an R-style instruction
 * with given arg1, arg2, and arg3, and
 * S1 and S2. */
static int write_r_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1, int s2)
{
	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	}

	if (is_invalid_reg(arg2)) {
		printf("Invalid register: %s at %d.\n",
				arg2, line);
		return -2;
	}

	if (is_invalid_reg(arg3)) {
		printf("Invalid register: %s at %d.\n",
				arg3, line);
		return -2;
	}

	set_opc(0, instr);
	set_s1(s1, instr);
	set_s2(s2, instr);
	decode_rsx(0, arg1, instr);
	decode_rsx(1, arg2, instr);
	decode_rsx(-1, arg3, instr);
	return 0;
}

/* Formats instr as an I-style instruction
 * with given arg1, arg2, and arg3, and S1. */
static int write_i_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3,
		int s1)
{
	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	}

	if (is_invalid_reg(arg2)) {
		printf("Invalid register: %s at %d.\n",
				arg2, line);
		return -2;
	}

	if (is_invalid_imm(arg3)) {
		printf("Invalid immediate: %s at %d.\n",
				arg3, line);
		return -2;
	}

	set_opc(1, instr);
	set_s1(s1, instr);
	decode_rsx(0, arg1, instr);
	decode_rsx(1, arg2, instr);
	decode_i_type_imm(arg3, instr);
	return 0;
}

/* Formats instr as an S-style instruction
 * with given arg1, arg2, and arg3. */
static int write_s_type(int line, uint16_t *instr,
		char *arg1, char *arg2, char *arg3)
{
	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	}

	if (is_invalid_reg(arg2)) {
		printf("Invalid register: %s at %d.\n",
				arg2, line);
		return -2;
	}

	if (is_invalid_imm(arg3)) {
		printf("Invalid immediate: %s at %d.\n",
				arg3, line);
		return -2;
	}

	set_opc(2, instr);
	decode_rsx(-1, arg1, instr);
	decode_rsx(1, arg2, instr);
	decode_s_type_imm(arg3, instr);
	return 0;
}

/* Formats instr as an U-style instruction
 * with given arg1 and arg2. */
static int write_u_type(int line, uint16_t *instr,
		char *arg1, char *arg2)
{
	if (is_invalid_reg(arg1)) {
		printf("Invalid register: %s at %d.\n",
				arg1, line);
		return -2;
	}

	if (is_invalid_imm(arg2)) {
		printf("Invalid immediate: %s at %d.\n",
				arg2, line);
		return -2;
	}

	set_opc(3, instr);
	decode_rsx(0, arg1, instr);
	decode_u_type_imm(arg2, instr);
	return 0;
}

/* Sets the OPC field of instr to the given value. */
static void set_opc(int opc, uint16_t *instr)
{
	set_bit(instr, 0, get_bit(opc, 0));
	set_bit(instr, 1, get_bit(opc, 1));
}

/* Sets the S1 field of instr to the given value. */
static void set_s1(int s1, uint16_t *instr)
{
	set_bit(instr, 5, get_bit(s1, 0));
	set_bit(instr, 6, get_bit(s1, 1));
	set_bit(instr, 7, get_bit(s1, 2));
}

/* Sets the S2 field of instr to the given value. */
static void set_s2(int s2, uint16_t *instr)
{
	set_bit(instr, 14, get_bit(s2, 0));
	set_bit(instr, 15, get_bit(s2, 1));
}

/* Makes sure that reg is a valid register. */
static int is_invalid_reg(char *reg)
{
	int i;
	for (i = 0; i < NUM_REGISTERS; i++) {
		if (!strcmp(reg, registers[i]))
			return 0;
	}
	return -2;
}

/* Given a valid register arg,
 * writes it to the rsX field of instr.
 * x < 0 ==> rs2
 * x = 0 ==> rs0
 * x > 0 ==> rs1	*/
static void decode_rsx(int x, char *arg, uint16_t *instr)
{
	int i;
	for (i = 0; i < NUM_REGISTERS; i++) {
		if (!strcmp(arg, registers[i]))
			break;
	}

	if (!x) {
		set_bit(instr, 2, get_bit(i, 0));
		set_bit(instr, 3, get_bit(i, 1));
		set_bit(instr, 4, get_bit(i, 2));
	} else if (x > 0) {
		set_bit(instr, 8, get_bit(i, 0));
		set_bit(instr, 9, get_bit(i, 1));
		set_bit(instr, 10, get_bit(i, 2));
	} else {
		set_bit(instr, 11, get_bit(i, 0));
		set_bit(instr, 12, get_bit(i, 1));
		set_bit(instr, 13, get_bit(i, 2));
	}
}

/* Makes sure that imm is a valid decimal number. */
static int is_invalid_imm(char *imm)
{
	int i;

	if (imm[0] != '-' && (imm[0] < '0' || imm[0] > '9'))
		return 1;

	for (i = 1; imm[i]; i++) {
		if (imm[i] < '0' || imm[i] > '9')
			return 1;
	}
	return 0;
}

/* Parses imm as a signed value, and writes
 * it to I-type instr appropriately. */
static void decode_i_type_imm(char *imm, uint16_t *instr)
{
	long imm_l = strtol(imm, NULL, 10);
	set_bit(instr, 11, get_bit(imm_l, 0));
	set_bit(instr, 12, get_bit(imm_l, 1));
	set_bit(instr, 13, get_bit(imm_l, 2));
	set_bit(instr, 14, get_bit(imm_l, 3));
	set_bit(instr, 15, get_bit(imm_l, 4));
}

/* Parses imm as a signed value, and writes
 * it to S-Type instr appropriately. */
static void decode_s_type_imm(char *imm, uint16_t *instr)
{
	long imm_l = strtol(imm, NULL, 10);
	set_bit(instr, 5, get_bit(imm_l, 0));
	set_bit(instr, 6, get_bit(imm_l, 1));
	set_bit(instr, 7, get_bit(imm_l, 2));
	set_bit(instr, 14, get_bit(imm_l, 3));
	set_bit(instr, 15, get_bit(imm_l, 4));
}

/* Parses imm as a signed value, and writes
 * it to U-Type instr appropriately. */
static void decode_u_type_imm(char *imm, uint16_t *instr)
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
}

/* Takes instr and converts it into a 4-digit
 * hex representation, stored in buf. */
static int instr_to_hex(uint16_t instr, char **buf)
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

/* returns the nth bit of x. */
static unsigned get_bit(uint16_t x, unsigned n)
{
	return x & (1U << n);
}

/* Sets the nth bit of x to v. */
static void set_bit(uint16_t *x, unsigned n, unsigned v)
{
	*x = (*x & ~(1U << n)) | (v << n);
}

