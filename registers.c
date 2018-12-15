#include "vm.h"
#include <string.h>
#include <inttypes.h>


// Interprets value as an immediate value or a retrieves the value from a register
uint16_t get_reg_immediate(registers * reg, uint16_t val) {
	uint16_t res;
	if(!VALID_REGISTER(val)) {
		return val;	
	}
	get_register(reg, val-32768, &res);
	return res;
}

// Writes to memory or a register depending on val range
void set_reg_memory(registers * reg, char * buffer, uint16_t * ch, uint16_t val) {
	if(!VALID_REGISTER(val)) {
		//printf("not a valid register\n");
		memcpy(buffer+VIRT_TO_PHYS(val), &ch, sizeof(uint16_t)); 
		return;
	}

	//printf("Setting reg %d to %d\n", val, *ch);
	set_register(reg, val-32768, ch, sizeof(uint16_t));
	return;
}

int set_register(registers * reg, int num, void * data, int len) {
	//printf("Setting reg %d to %d\n", num, *(uint16_t *)data);
	if(!memcpy(((char*)reg) + REG_SIZE_BYTES*num, data, len > 2 ? 2 : len)) {
		return -1;
	}	
	return 0;
}

int get_register(registers * reg, int num, void * data) {
	if(!memcpy(data, (char *)reg + REG_SIZE_BYTES*num, REG_SIZE_BYTES)) {
		return -1;
	}
	return 0;
}
