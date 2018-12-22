#include "vm.h"
#include <string.h>
#include <stdlib.h>

/*
 * Initialize a stack of initial size 'size'
 */
stack_info * stack_init(int size) {

	stack_info * stack = malloc(sizeof(struct stack_info));

	if(!stack) {
		return NULL;
	}

	stack->total_size = size;
	stack->num_elements = 0;
	stack->mem = malloc(size);
	
	if(!stack->mem) {
		return NULL;
	}

	return stack;
}


/*
 * Push an item onto the stack from a register, resize if necessary
 */
int push_stack(stack_info * stack, registers * reg, void * data, int reg_num) {
	
	char * old_mem;
	
	// check resize
	old_mem = stack->mem;
	if(stack->total_size < REG_SIZE_BYTES*(stack->num_elements+1)) {
		stack->mem = realloc(stack->mem, stack->total_size + REG_SIZE_BYTES*STACK_RESIZE);
		stack->total_size += REG_SIZE_BYTES*STACK_RESIZE;
		if(stack->mem == NULL) {
			stack->mem = old_mem;
			return -1;
		}
	}

	// push register onto stack
	if(reg != NULL && reg_num < 8 && reg_num >= 0)
		get_register(reg, reg_num, stack->mem + REG_SIZE_BYTES*stack->num_elements); 
	else if (data != NULL) 
		memcpy(stack->mem + (stack->num_elements)*REG_SIZE_BYTES, data, REG_SIZE_BYTES);
	stack->num_elements++;
	return 0;
}



/*
 * Pop an item from the stack into a register
 */ 
int pop_stack(stack_info * stack, registers * reg, uint16_t * val, int reg_num) {
	if(stack->num_elements <=0) {
		return -1;
	}
	
	if(!val) {	
		set_register(reg, reg_num, stack->mem + REG_SIZE_BYTES*(stack->num_elements-1), REG_SIZE_BYTES);
		stack->num_elements--;
		return 0;
	}
	memcpy(val, stack->mem + REG_SIZE_BYTES*(stack->num_elements-1), sizeof(uint16_t));
	stack->num_elements--;
	return 0;
}

/*
 * Cleanup stack memory
 */
void cleanup_stack(stack_info * stack) {
	free(stack->mem);
	free(stack);
}


// print stack for debugging
void print_stack(stack_info * stack) {
	for (int i = 0; i < stack->num_elements; i++) {
		printf("%d:  %04X\n",i, *(uint16_t*)(stack->mem+(i*REG_SIZE_BYTES)));
	}
}
