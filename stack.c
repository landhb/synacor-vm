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
 * Push an item onto the stack, resize if necessary
 */
int push_stack(stack_info * stack, void * data) {
	
	char * old_mem;
	
	// check resize
	old_mem = stack->mem;
	if(stack->total_size < REG_SIZE*(stack->num_elements+1)) {
		stack->mem = realloc(stack->mem, stack->total_size + REG_SIZE*STACK_RESIZE);
		stack->total_size += REG_SIZE*STACK_RESIZE;
		if(stack->mem == NULL) {
			stack->mem = old_mem;
			return -1;
		}
	}

	// push onto stack
	memcpy(stack->mem + REG_SIZE*stack->num_elements, data, REG_SIZE);
	stack->num_elements++;
	return 0;
}



/*
 * Pop an item from the stack
 */ 
int pop_stack(stack_info * stack, void * return_data) {
	memcpy(return_data, stack->mem + REG_SIZE*(stack->num_elements-1), REG_SIZE);
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
