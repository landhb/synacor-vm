#include "vm.h"
#include <stdlib.h>

// I/O
extern FILE * in;
extern FILE * out;


int parse_file(FILE * f, registers * reg, stack_info * stack) {
	
	int i;
	char* buffer;
	unsigned long file_len;
	
	// get filesize
	fseek(f, 0, SEEK_END);
	file_len=ftell(f);
	fseek(f, 0, SEEK_SET);
	if(file_len <= 0) {
		return -1;
	}

	// create memory space for the "process"
	buffer = malloc(VM_MEM_SIZE);
	if(!buffer) {
		return -1;
	}

	// read the entire file into memory
	fread(buffer, file_len, 1, f);
	fclose(f);
	printf("Read %lu bytes from disk\n", file_len);

	// set io redirection
	in = stdin;
	out = stdout;

	// step through instructions
	i=0;
	while(i < file_len) {
		i+=run_instruction(buffer, i, reg, stack);	
	}

	return 0;
}

int main(int argc, char **argv) {

	FILE * file;
	stack_info * stack;
	registers * reg;

	// open binary file
	file = fopen(argv[1], "rb");


	// init stack + registers
	stack = stack_init(INIT_STACK_SIZE);
	reg = malloc(sizeof(struct registers));

	// run binary
	parse_file(file, reg, stack);

	fclose(file);
}
