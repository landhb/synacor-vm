#include "vm.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


int parse_file(FILE * f, registers * reg, stack_info * stack) {
	
	int i;
	char * buffer;
	unsigned long file_len;
	
	// get filesize
	fseek(f, 0, SEEK_END);
	file_len=ftell(f);
	fseek(f, 0, SEEK_SET);
	if(file_len <= 0) {
		return -1;
	}

	// create memory space for the "process"
	buffer = malloc(file_len);
	if(!buffer) {
		return -1;
	}

	// read the entire file into memory
	fread(buffer, file_len, 1, f);
	fclose(f);

	// step through instructions
	i=0;
	while(i < file_len) {
		i+=run_instruction(buffer, i, reg, stack);	
	}

	return 0;
}



int run_instruction(char * buffer, int i, registers * reg, stack_info * stack) {
	switch(GET_RIGHTMOST_16_BITS(buffer[i])) {
		
		case 0: //halt
			exit(0);
		
		case 1: // set register <a> to <b>
			set_register(reg, buffer[i+REG_SIZE_BYTES], &(buffer[i+2*REG_SIZE_BYTES]), REG_SIZE_BYTES);
			return REG_SIZE_BYTES*3;
		
		case 2: // push <a>
			if(*(uint16_t*)(buffer+i+REG_SIZE_BYTES) < 32767) {
				// push memory
				push_stack(stack, NULL, &buffer[i+REG_SIZE_BYTES], 0);
			}
			else if (*(uint16_t*)(buffer+i+REG_SIZE_BYTES) <= 32775) {
				// push register
				push_stack(stack, reg, NULL,*(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768);
			} 
			// TODO? exit on invalid address/register
			return REG_SIZE_BYTES*2;
		
		default: // invalid instruction
			exit(-1);

	}
}
