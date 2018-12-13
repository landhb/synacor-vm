#ifndef __SYNACOR_VM__
#define __SYNACOR_VM__

#include <stdint.h>
#include <stdio.h>

// architecture constants
#define ADDRESS_SIZE 15
#define REG_SIZE 16
#define REG_SIZE_BYTES REG_SIZE/8

// stack constants
#define INIT_STACK_SIZE 80
#define STACK_RESIZE 3

#define GET_RIGHTMOST_16_BITS(x) ((x) & (unsigned int)0x0000FFFF)
#define VALID_REGISTER(x) ((x) <= 32775 && (x) >= 32768)
#define VALID_MEMORY(x) ((x) <= 32767 && (x) >= 0)
#define VIRT_TO_PHYS(x) ((x)

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

// stack primatives
stack_info * stack_init(int size);
int push_stack(stack_info * stack, registers * reg, void * data,int reg_num);
int pop_stack(stack_info * stack, registers * reg, int reg_num);
void cleanup_stack(stack_info * stack);
void print_stack(stack_info * stack);

// registers primatives
int set_register(registers * reg, int num, void * data, int len);
int get_register(registers * reg, int num, void * data);
uint16_t get_reg_immediate(registers * reg, uint16_t val);

// instructions
int run_instruction(char * buffer, int i, registers * reg, stack_info * stack);
int parse_file(FILE *f, registers * reg, stack_info* stack);
#endif // __SYNACORE_VM__ 
