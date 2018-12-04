#include "../vm.h"
#include "snow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

describe(registers) {
	it("write/read registers") {
		void * buf = malloc(REG_SIZE+1);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(buf, NULL);
		assertneq(reg,NULL);

		// set registers
		asserteq(set_register(reg, 1, "dead", strlen("dead")),0);
		asserteq(set_register(reg, 2, "beef", strlen("beef")),0);
		
		// verify first
		asserteq(get_register(reg, 1, buf), 0);
		asserteq_buf(buf, "dead", REG_SIZE/8);

		// verify second	
		asserteq(get_register(reg, 2, buf), 0);
		asserteq_buf(buf, "beef", REG_SIZE/8); 
		defer(free(reg));
		defer(free(buf));
	}

}

describe(stack) {
	it("stack initializes") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		assertneq(stack, NULL);
		defer(cleanup_stack(stack));
	}


	subdesc(stack_functions) {
		it("push onto stack") {
			stack_info * stack = stack_init(INIT_STACK_SIZE);
			assertneq(stack, NULL);
	
			// test data
			char * tmp;
			tmp = calloc(1,REG_SIZE);
			memcpy(tmp, "12345678912345600", REG_SIZE-1);
		
			// push onto stack
			asserteq(push_stack(stack,tmp),0);
			asserteq_buf(stack->mem, tmp, REG_SIZE);
			defer(cleanup_stack(stack));
			defer(free(tmp));
		}

		it("pop off stack") {
			stack_info * stack = stack_init(INIT_STACK_SIZE);
			assertneq(stack, NULL);
	
			// test data
			char * tmp;
			tmp = calloc(1,REG_SIZE);
			memcpy(tmp, "12345678912345600", REG_SIZE-1);
		
			// push onto stack
			asserteq(push_stack(stack,tmp),0);
			asserteq_buf(stack->mem, tmp, REG_SIZE);

			// clear buff
			bzero(tmp, REG_SIZE);
			assertneq_buf(stack->mem, tmp, REG_SIZE);

			// pop off stack
			pop_stack(stack, tmp);
			asserteq_buf(stack->mem, tmp, REG_SIZE);
			defer(cleanup_stack(stack));
			defer(free(tmp));
		}
		it("stack resizes") {
			stack_info * stack = stack_init(INIT_STACK_SIZE);
			assertneq(stack, NULL);
	
			// test data
			char * tmp;
			tmp = calloc(1,REG_SIZE);
			memcpy(tmp, "12345678912345600", REG_SIZE-1);
		
			// push > INIT_STACK_SIZE data onto stack
			while(stack->total_size <= INIT_STACK_SIZE + REG_SIZE*STACK_RESIZE*2) {
				asserteq(push_stack(stack,tmp), 0);
				asserteq_buf(stack->mem, tmp, REG_SIZE);
			}

			// clear stack
			while(stack->num_elements > 0) {
				// clear buff
				bzero(tmp, REG_SIZE);
				assertneq_buf(stack->mem, tmp, REG_SIZE);

				// pop off stack
				pop_stack(stack, tmp);
				asserteq_buf(stack->mem, tmp, REG_SIZE);
			}
			defer(cleanup_stack(stack));
			defer(free(tmp));	
		}
	}
}

snow_main();


