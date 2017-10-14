/*
   Takes a file with bal commdands and turns it into
   a file with equivalent bml instructions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Parses bal code in source into bml code in dest. */
parse(char *src, char *dst);

/* Writes the bal commands in src_lines into dst as
 * bml instructions readable by logisim. */
/* TODO */
write_bml(char *dst, char **src_lines);

/* Reads the contents of src and puts it into buf. */
/* TODO */
read_file(char *src, char **buf);

/* Reads the lines of src and puts a null-terminated 
 * list of its lines into lines. */
/* TODO */
separate_lines(char *src, char ***lines);

/* Frees a null-terminated list of strings. */
/* TODO */
free_str_lst(char **str_lst);

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

	failure = parse(argv[1], argv[2]);

end:

	/* Handles errors and tells the user what happened. */
	if (failure)
		printf("Parsing failed, neither file has been changed.\n");
		printf("Error code: %d\n", failure);

	else 
		printf("Parsing successful!\n");
		printf("%s now contains bad machine langauge!\n", argv[2]);

	return failure;
}

/* Parses bal code in source into bml code in dest. */
int parse(char *src, char *dst)
{
	int failure;

	char *src_buf;
	char **src_lines;

	/* Reads the contents of src into src_buf. */
	failure = read_file(source, &src_buf);
	if (failure) {
		printf("Failed to read %s at %d.\n",
				src, __LINE__ - 2);

		return failure;
	}

	/* Separates src_buf into lines (commands) and puts
	 * them into a null-terminated list src_lines. */
	failure = separate_lines(src_buf, &src_lines);
	if (failure) {
		printf("Failed to separate %s into lines at %d.\n",
				src, __LINE__ - 2);

		goto free_src_lines;
	}
	
	/* Writes the commands (in src_lines) into
	 * dst, finishing the process. */
	failure = write_bml(dst, src_lines);
	if (failure)
		printf("Failed to write bml to %s at %d.\n",
				dst, __LINE__ - 2);
	
	/* Frees things. */
	free(src_buf);
free_src_lines:
	free_str_lst(src_lines);

	return failure;
}

