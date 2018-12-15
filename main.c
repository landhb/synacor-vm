#include "vm.h"
#include <stdlib.h>

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
