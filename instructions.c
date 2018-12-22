#include "vm.h"
#include <stdlib.h>
#include <string.h>

// global stdin/stdout redirection
FILE * in;
FILE * out;

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
		//printf("ip @ %d, inst: %d\n", i, (unsigned short)buffer[i]);
		i+=run_instruction(buffer, i, reg, stack);	
	}

	return 0;
}


int run_instruction(char * buffer, int i, registers * reg, stack_info * stack) {
	uint16_t a = 0;
	uint16_t b = 0;
	uint16_t c = 0;
	int ch = 0;
	switch(GET_RIGHTMOST_16_BITS(buffer[i])) {
		
		case 0: //halt
			exit(0);
		
		case 1: // set register <a> to <b>
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
                        if(!VALID_REGISTER(a)) {
				exit(-1);
			}
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			set_register(reg, a-32768, &b, REG_SIZE_BYTES);
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
				
				pop_stack(stack, reg, NULL,*(uint16_t*)(buffer+i+REG_SIZE_BYTES)-32768); 
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
			return ((*(int16_t*)(buffer+i+REG_SIZE_BYTES))*REG_SIZE_BYTES)-i;

		case 7: // jt <a> <b> (if a !=0 jmp <b>)
			a = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES));
			if(a == 0)
				return REG_SIZE_BYTES*3;
			if (!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES*2))) {
                                exit(-1);
                        }
			return ((*(int16_t*)(buffer+i+REG_SIZE_BYTES*2))*REG_SIZE_BYTES)-i;
		
		case 8: // jf <a> <b> (if a == 0 jmp <b>)
			a = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES));
                        if(a != 0) 
                                return REG_SIZE_BYTES*3;
                        if (!VALID_MEMORY(*(uint16_t*)(buffer+i+REG_SIZE_BYTES*2))) {
                                exit(-1);
                        }
                        return ((*(int16_t*)(buffer+i+REG_SIZE_BYTES*2))*REG_SIZE_BYTES)-i;

		case 9: // add <a> <b> <c>
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b+c)%32768u;
			set_reg_memory(reg, buffer, &b,a);
			return REG_SIZE_BYTES*4;
		case 10: // mult <a> <b> <c>
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b*c)%32768u;
			set_reg_memory(reg,buffer, &b,a);
			return REG_SIZE_BYTES*4;
		case 11: // mod <a> <b> <c>
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = (b%c);
			set_reg_memory(reg, buffer, &b, a);
			return REG_SIZE_BYTES*4;
		case 12: // and <a> <b> <c>
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = b & c;
			set_reg_memory(reg,buffer,&b,a);
			return REG_SIZE_BYTES*4;
		case 13: // or <a> <b> <c>
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			c = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*3));
			b = b|c;
			set_reg_memory(reg, buffer,&b,a);
			return REG_SIZE_BYTES*4;
		case 14: // not <a> <b> 
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			b = (~b) & 0x7fff;
			set_reg_memory(reg, buffer, &b, a);
			return REG_SIZE_BYTES*3;
		case 15: // rmem <a> <b> 
			a = *(uint16_t*)(buffer+i+REG_SIZE_BYTES);
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			if(!VALID_MEMORY(b)) {
				exit(-1);
			}
			memcpy(&b, buffer + VIRT_TO_PHYS(b), REG_SIZE_BYTES); 
			set_reg_memory(reg, buffer, &b,a);
			return REG_SIZE_BYTES*3;
		case 16: // wmem <a> <b>
			a = get_reg_immediate(reg,*(uint16_t*)(buffer+i+REG_SIZE_BYTES));
			b = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES*2));
			if(!VALID_MEMORY(a)) {
				exit(-1);
			}
			memcpy(buffer+VIRT_TO_PHYS(a), &b,REG_SIZE_BYTES); 
			return REG_SIZE_BYTES*3;
		case 17: // call <a>
			a = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES));
			if (!VALID_MEMORY(a)) {
				exit(-1);
			}
			b = i + REG_SIZE_BYTES*2;
			b /= REG_SIZE_BYTES; // next instruction
			push_stack(stack, NULL, &b, sizeof(uint16_t));
			return (a*REG_SIZE_BYTES)-i;
		case 18: // ret 
			if(pop_stack(stack,NULL,&a,0) < 0)
				exit(-1);
			return (a*REG_SIZE_BYTES)-i;	
		case 19: // out <a> 
			a = get_reg_immediate(reg, *(uint16_t*)(buffer+i+REG_SIZE_BYTES));
			fprintf(out, "%c",a);
			fflush(out);
			return REG_SIZE_BYTES*2;
		case 20: // in <a>
			//ch = getchar();
			ch = fgetc(in);
			if (ch == EOF) {
				exit(-1);
			}
			set_reg_memory(reg, buffer, (uint16_t*)&ch, 1);
			return REG_SIZE_BYTES*2;
		case 21: // nop
			return REG_SIZE_BYTES;		
		default: // invalid instruction
			fprintf(stderr, "Invalid instruction\n");
			exit(-1);

	}
}
