#include "vm.h"
#include <stdlib.h>


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
	printf("Read %lu bytes from disk\n", file_len);

	// step through instructions
	i=0;
	while(i < file_len) {
		//printf("ip @ %d\n", i);
		i+=run_instruction(buffer, i, reg, stack);	
	}

	return 0;
}




int run_instruction(char * buffer, int i, registers * reg, stack_info * stack) {
	uint16_t a = 0;
	uint16_t b = 0;
	uint16_t c = 0;
	//printf("\tinst: %d\n",GET_RIGHTMOST_16_BITS(buffer[i]));		
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
	
		case 3: // pop <a>
			if(VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				pop_stack(stack, reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768); 
			}
			// TODO? exit on invalid register
			return REG_SIZE_BYTES*2;			

		case 4: // eq a b c (set register a to 1 if b = c)
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));	
			set_register(reg, a, b==c ? "\x01\x00" : "\x00\x00", REG_SIZE_BYTES);
			return REG_SIZE_BYTES*4;
	
		case 5: // gt a b c (set register a to 1 if b > c)
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			set_register(reg, a, b > c ? "\x01\x00" : "\x00\x00", REG_SIZE_BYTES);
			return REG_SIZE_BYTES*4;
		
		case 6: // jmp <a>
			if (!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			return (*(int16_t*)(buffer+i+REG_SIZE_BYTES))-i;

		case 7: // jt <a> <b> (if a !=0 jmp <b>)
			a = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES));
			if(a == 0)
				return REG_SIZE_BYTES*3;
			if (!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES*2))) {
                                exit(-1);
                        }
			return (*(int16_t*)(buffer+i+REG_SIZE_BYTES*2))-i;
		
		case 8: // jf <a> <b> (if a == 0 jmp <b>)
			a = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES));
                        if(a != 0) 
                                return REG_SIZE_BYTES*3;
                        if (!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES*2))) {
                                exit(-1);
                        }
                        return (*(int16_t*)(buffer+i+REG_SIZE_BYTES*2))-i;

		case 9: // add <a> <b> <c>
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b+c)%32768;
			set_register(reg, a, &b, sizeof(uint16_t));
			return REG_SIZE_BYTES*4;
		case 10: // mult <a> <b> <c>
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b*c)%32768;
			set_register(reg, a, &b, sizeof(uint16_t));
			return REG_SIZE_BYTES*4;
		case 11: // mod <a> <b> <c>
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b%c)%32768;
			set_register(reg, a, &b, sizeof(uint16_t));
			return REG_SIZE_BYTES*4;
		case 12: // and <a> <b> <c>
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b&c)%32768;
			set_register(reg, a, &b, sizeof(uint16_t));
			return REG_SIZE_BYTES*4;
		case 13: // or <a> <b> <c>
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b|c)%32768;
			set_register(reg, a, &b, sizeof(uint16_t));
			return REG_SIZE_BYTES*4;
		case 14: // not <a> <b> 
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			b = ~b;
			set_register(reg, a, &b, sizeof(uint16_t));
			return REG_SIZE_BYTES*3;
		case 15: // rmem <a> <b> 
			if(!VALID_REGISTER(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768;
			if(!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES*2))) {
				exit(-1);
			}
			b = *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2);
			set_register(reg, a, &b, sizeof(uint16_t));
			return REG_SIZE_BYTES*3;
		case 16: // wmem <a> <b>
			if(!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			//memcpy(buffer+a, 
			return REG_SIZE_BYTES*3;
		case 17:
			if (!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES))) {
				exit(-1);
			}
			b = i + REG_SIZE_BYTES*2;
			push_stack(stack, NULL, &b, sizeof(uint16_t));
			return (*(int16_t*)(buffer+i+REG_SIZE_BYTES))-i;
		case 18:
			pop_stack(
		case 19: 
			a = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES));
			fprintf(stdout, "%c",a);
			return REG_SIZE_BYTES*2;
		case 20:
			
		case 21: // nop
			return REG_SIZE_BYTES;		
		default: // invalid instruction
			fprintf(stderr, "Invalid instruction\n");
			exit(-1);

	}
}
