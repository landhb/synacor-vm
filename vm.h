#ifndef __SYNACOR_VM__
#define __SYNACOR_VM__

// architecture constants
#define ADDRESS_SIZE 15
#define REG_SIZE 16

// stack constants
#define INIT_STACK_SIZE 80
#define STACK_RESIZE 3

/* 
 * Memory abstraction
 */ 
typedef struct vm_table_entry {

	// address	
	unsigned addr:15;

	// value
	unsigned val:16;

} vm_table_entry;

/*
 * Stack abstraction
 *
 */
typedef struct stack_info {
	char * mem;
	int total_size;
	int num_elements;
} stack_info;

/*
 * Registers - 8 total
 * 	
 */
typedef struct registers {
	unsigned r0:15;
	unsigned r1:15;
	unsigned r2:15;
	unsigned r3:15;
	unsigned r4:15;
	unsigned r5:15;
	unsigned r6:15;
	unsigned r7:15;
} registers;


// stack.c
stack_info * stack_init(int size);
int push_stack(stack_info * stack, void * data);
int pop_stack(stack_info * stack, void * return_data);
void cleanup_stack(stack_info * stack);

#endif 
