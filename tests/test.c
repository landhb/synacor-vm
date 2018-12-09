#include "../vm.h"
#include "snow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // fork, getpid, exec
#include <sys/types.h> //pid_t datatype
#include <sys/wait.h> //waitpid()

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
		asserteq_buf(buf, "dead", REG_SIZE_BYTES);
		
		// verify second	
		asserteq(get_register(reg, 2, buf), 0);
		asserteq_buf(buf, "beef", REG_SIZE_BYTES); 
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
			registers * reg = malloc(sizeof(struct registers));
			assertneq(stack, NULL);
	
			// test data
			char * tmp;
			tmp = calloc(1,REG_SIZE);
			memcpy(tmp, "12345678912345600", REG_SIZE_BYTES);
			set_register(reg, 0, tmp, REG_SIZE_BYTES);

			// push onto stack
			asserteq(push_stack(stack,reg, 0),0);
			asserteq_buf(stack->mem, tmp, REG_SIZE_BYTES);
			defer(cleanup_stack(stack));
			defer(free(reg));
			defer(free(tmp));
		}

		it("pop off stack") {
			stack_info * stack = stack_init(INIT_STACK_SIZE);
			registers * reg = malloc(sizeof(struct registers));
			assertneq(stack, NULL);
	
			// test data
			char * tmp, *res;
			tmp = calloc(1,REG_SIZE);
			res = calloc(1,REG_SIZE);
			memcpy(tmp, "12345678912345600", REG_SIZE-1);
			set_register(reg, 0, tmp, REG_SIZE_BYTES);
			
			// push tmp onto stack
			asserteq(push_stack(stack, reg, 0),0);
			asserteq_buf(stack->mem, tmp, REG_SIZE_BYTES);

			// pop off stack into res
			pop_stack(stack, reg, 1);  // pop stack item into r1
			get_register(reg, 1, res); // place r1 into res
			asserteq_buf(res, tmp, REG_SIZE_BYTES); //cmp res to tmp
			defer(cleanup_stack(stack));
			defer(free(reg));
			defer(free(tmp));
			defer(free(res));
		}
		it("stack resizes") {
			stack_info * stack = stack_init(INIT_STACK_SIZE);
			registers * reg = malloc(sizeof(struct registers));
			assertneq(stack, NULL);
	
			// test data
			char * tmp, *res;
			tmp = calloc(1,REG_SIZE);
			res = calloc(1,REG_SIZE);
			memcpy(tmp, "12345678912345600", REG_SIZE-1);
		
			// push > INIT_STACK_SIZE data onto stack
			while(stack->total_size <= INIT_STACK_SIZE + REG_SIZE_BYTES*STACK_RESIZE*2) {
				set_register(reg, 0, tmp, REG_SIZE_BYTES);
				asserteq(push_stack(stack,reg,0), 0);
				asserteq_buf(stack->mem, tmp, REG_SIZE_BYTES);
			}

			// clear stack
			while(stack->num_elements > 0) {

				// pop off stack
				pop_stack(stack, reg, 1);
				get_register(reg, 1, res);
				asserteq_buf(res, tmp, REG_SIZE_BYTES);
			}
			defer(cleanup_stack(stack));
			defer(free(reg));
			defer(free(tmp));
			defer(free(res));	
		}
	}
}

describe(instructions) {
	it("halt") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(4);
		memcpy(buf, "\x00\x00\x01\x02", 4);

		// fork child to test halt() instruction
		pid_t pid = fork();
		int status= -1;
		
		// child will run the instruction
		if(pid==0) {
			run_instruction(buf, 0, reg, stack);	
		}
		else {
			// assert that child exited properly
			waitpid(pid, &status, 0);
			asserteq(WIFEXITED(status),1);
		}
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));
	}
	it("set") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf,*res;
		buf = malloc(6);
		res = malloc(REG_SIZE_BYTES);
		memcpy(buf, "\x01\x00\x01\x00\x04\x00", 6);
		
		// should return 6
		asserteq(run_instruction(buf, 0, reg, stack),6);	
		
		// test if register was set
		asserteq(get_register(reg, 1, res), 0);
		asserteq_buf(res, "\x04\x00", REG_SIZE_BYTES);
		
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
		defer(free(res));
	}
	it("push") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf,*res;
		buf = malloc(6);
		res = malloc(REG_SIZE_BYTES);
		memcpy(buf, "\x02\x00\x01\x00\x04\x00", 6);
		
		// should return 4
		asserteq(run_instruction(buf, 0, reg, stack),4);	
		
		// test if register was set
		asserteq(get_register(reg, 1, res), 0);
		asserteq_buf(res, "\x04\x00", REG_SIZE_BYTES);
		
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
		defer(free(res));	
	}

}

snow_main();


