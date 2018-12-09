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
			asserteq(push_stack(stack,reg, NULL,0),0);
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
			asserteq(push_stack(stack, reg, NULL,0),0);
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
				asserteq(push_stack(stack,reg,NULL,0), 0);
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
		memcpy(buf, "\x00\x00\x01\x02", 4); // halt

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
		memcpy(buf, "\x01\x00\x01\x00\x04\x00", 6); // set 0x0001 0x0004
		
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
		char *buf;
		buf = malloc(6);
		memcpy(buf, "\x02\x00\x01\x00\x04\x00", 6); // push 0x0001
		
		// first test pushing data directly to stack, should return 4
		asserteq(run_instruction(buf, 0, reg, stack),4);	
		
		// test if \x01\x00 is on the stack 
		asserteq_buf(stack->mem, "\x01\x00", 2);

		// now test pushing from register, should still return 4
		memcpy(buf, "\x02\x00\x06\x80", 4); // push 0x8006 -> push r6
		set_register(reg, 6, "te", REG_SIZE_BYTES);
		asserteq(run_instruction(buf, 0, reg, stack),4);	
		asserteq_buf(stack->mem+(stack->num_elements-1)*REG_SIZE_BYTES, "te", REG_SIZE_BYTES);
	
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("pop") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(6);
		memcpy(buf, "\x03\x00\x01\x80", 4); //pop r1
		
		// push test data onto the stack
		push_stack(stack, NULL, "te", REG_SIZE_BYTES);

		// pop data into register 1, should return 4
		asserteq(run_instruction(buf, 0, reg, stack),4);	
		
		// test that register contains test data
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "te", REG_SIZE_BYTES);
	
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("eq") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x04\x00\x01\x80\x01\x00\x01\x00", 8); // eq r1 0x00001 0x00001
		
		// test 0x0001 == 0x0001, should set r1=1 and return 8
		asserteq(run_instruction(buf, 0, reg, stack),8);	
		
		// test that register contains 0x01
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x01\x00", REG_SIZE_BYTES);

		// test not equal
		memcpy(buf, "\x04\x00\x01\x80\x01\x00\x02\x00", 8); // eq r1 0x00001 0x00002
		asserteq(run_instruction(buf, 0, reg, stack),8);	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x00\x00", REG_SIZE_BYTES);

		// test register comparison to data
		memcpy(buf, "\x04\x00\x01\x80\x01\x00\x02\x80", 8); // eq r1 0x00001 r2
		set_register(reg, 2, "\x01\x00", REG_SIZE_BYTES);
		asserteq(run_instruction(buf, 0, reg, stack),8);	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x01\x00", REG_SIZE_BYTES);


		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("gt") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x05\x00\x01\x80\x02\x00\x01\x00", 8); // gt r1 0x00002 0x00001
		
		// test 0x0002 > 0x0001, should set r1=1 and return 8
		asserteq(run_instruction(buf, 0, reg, stack),8);	
		
		// test that register contains 0x01
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x01\x00", REG_SIZE_BYTES);

		// test not greater than
		memcpy(buf, "\x05\x00\x01\x80\x01\x00\x01\x00", 8); // gt r1 0x00001 0x00001
		asserteq(run_instruction(buf, 0, reg, stack),8);	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x00\x00", REG_SIZE_BYTES);

		// test register comparison to data
		memcpy(buf, "\x05\x00\x01\x80\x02\x00\x02\x80", 8); // gt r1 0x00002 r2
		set_register(reg, 2, "\x01\x00", REG_SIZE_BYTES);
		asserteq(run_instruction(buf, 0, reg, stack),8);	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x01\x00", REG_SIZE_BYTES);


		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("jmp") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(0x20);
		memcpy(buf, "\x06\x00\x10\x00", 4); // jmp 0x0010
		
		// should return 0x0010-0 = 16
		asserteq(run_instruction(buf, 0, reg, stack),16);	

		// test jumping back, should return 0x0010-0x0005 = -11	
		memcpy(buf+0x0010, "\x06\x00\x05\x00", 4); // jmp 0x0005
		asserteq(run_instruction(buf, 0x0010, reg, stack),-11);	
	
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
}

snow_main();


