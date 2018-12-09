#include "vm.h"
#include <string.h>

int set_register(registers * reg, int num, void * data, int len) {
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
