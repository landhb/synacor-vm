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
struct registers {
	unsigned r0:REG_SIZE;
	unsigned r1:REG_SIZE;
	unsigned r2:REG_SIZE;
	unsigned r3:REG_SIZE;
	unsigned r4:REG_SIZE;
	unsigned r5:REG_SIZE;
	unsigned r6:REG_SIZE;
	unsigned r7:REG_SIZE;
} __attribute__( ( packed ) ); 
typedef struct registers registers;

// stack.c
stack_info * stack_init(int size);
int push_stack(stack_info * stack, void * data);
int pop_stack(stack_info * stack, void * return_data);
void cleanup_stack(stack_info * stack);


// registers.c
int set_register(registers * reg, int num, void * data, int len);
int get_register(registers * reg, int num, void * data);
#endif 
