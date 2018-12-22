#include "../vm.h"
#include "snow.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // fork, getpid, exec
#include <sys/types.h> //pid_t datatype
#include <sys/wait.h> //waitpid()

extern FILE * in;
extern FILE * out;

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
			pop_stack(stack, reg, NULL, 1);  // pop stack item into r1
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
				pop_stack(stack, reg, NULL,1);
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
		memcpy(buf, "\x01\x00\x01\x80\x04\x00", 6); // set r1 0x0004
		
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
		buf = malloc(0x20*REG_SIZE_BYTES);
		memcpy(buf, "\x06\x00\x10\x00", 4); // jmp 0x0010
		
		// should return 0x0010-0 = 16*REG_SIZE_BYTES
		asserteq(run_instruction(buf, 0, reg, stack),16*REG_SIZE_BYTES);	

		// test jumping back, should return 0x0010-0x0005 = -11*REG_SIZE_BYTES	
		memcpy(buf+0x0010*REG_SIZE_BYTES, "\x06\x00\x05\x00", 4); // jmp 0x0005
		asserteq(run_instruction(buf, 0x0010*REG_SIZE_BYTES, reg, stack),-11*REG_SIZE_BYTES);	
	
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("jt") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(0x20*REG_SIZE_BYTES);
		memcpy(buf, "\x07\x00\x01\x00\x10\x00", 6); // jt 0x0001 0x0010
		
		// should jump, return 0x0010-0 = 16*REG_SIZE_BYTES
		asserteq(run_instruction(buf, 0, reg, stack),16*REG_SIZE_BYTES);	

		// test <a>=0, should not jump, return 6	
		memcpy(buf+0x0010*REG_SIZE_BYTES, "\x07\x00\x00\x00\x05\x00", 4); 
		asserteq(run_instruction(buf, 0x0010*REG_SIZE_BYTES, reg, stack),6);	
	
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("jf") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(0x20*REG_SIZE_BYTES);
		memcpy(buf, "\x08\x00\x00\x00\x10\x00", 6); // jf 0x0000 0x0010
		
		// should jump, return 0x0010-0 = 16*REG_SIZE_BYTES
		asserteq(run_instruction(buf, 0, reg, stack),16*REG_SIZE_BYTES);	

		// test <a>!=0, should not jump, return 6	
		memcpy(buf+0x0010*REG_SIZE_BYTES, "\x08\x00\x01\x00\x05\x00", 4); 
		asserteq(run_instruction(buf, 0x0010*REG_SIZE_BYTES, reg, stack),6);	
	
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("add") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x09\x00\x01\x80\x01\x00\x10\x00", 8); // add r1 0x0001 0x0010
		
		// should return REG_SIZE_BYTES*4
		asserteq(run_instruction(buf, 0, reg, stack),4*REG_SIZE_BYTES);	

		// test r1 = 0x01 + 0x010 = 0x11	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x11\x00", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("mult") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x0a\x00\x01\x80\x02\x00\x10\x00", 8); // mult r1 0x0002 0x0010
		
		// should return REG_SIZE_BYTES*4
		asserteq(run_instruction(buf, 0, reg, stack),4*REG_SIZE_BYTES);	

		// test r1 = 0x02 * 0x010 = 0x20	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x20\x00", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("mod") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x0b\x00\x01\x80\x11\x00\x02\x00", 8); // mod r1 0x0011 0x0002 

		// should return REG_SIZE_BYTES*4
		asserteq(run_instruction(buf, 0, reg, stack),4*REG_SIZE_BYTES);	

		// test r1 = 0x11 % 0x02 = 0x01	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x01\x00", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("and") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x0c\x00\x01\x80\x11\x00\x03\x00", 8); // and r1 0x0011 0x0003 

		// should return REG_SIZE_BYTES*4
		asserteq(run_instruction(buf, 0, reg, stack),4*REG_SIZE_BYTES);	

		// test r1 = 0x11 & 0x03 = 0x01	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x01\x00", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("or") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x0d\x00\x01\x80\x11\x00\x02\x00", 8); // or r1 0x0011 0x0002 

		// should return REG_SIZE_BYTES*4
		asserteq(run_instruction(buf, 0, reg, stack),4*REG_SIZE_BYTES);	

		// test r1 = 0x11 | 0x02 = 0x13	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x13\x00", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("not") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x0e\x00\x01\x80\x11\x11", 6); // not r1 0x1111 

		// should return REG_SIZE_BYTES*3
		asserteq(run_instruction(buf, 0, reg, stack),3*REG_SIZE_BYTES);	

		// test r1 = ~0x1111 & 0x7fff = 0x6EEE 	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\xEE\x6E", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("rmem") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x0f\x00\x01\x80\x01\x00", 6); // rmem r1 0x1 

		// should return REG_SIZE_BYTES*3
		asserteq(run_instruction(buf, 0, reg, stack),3*REG_SIZE_BYTES);	

		// test r1 = buf[2-4] 	
		asserteq(get_register(reg, 1, buf),0);
		asserteq_buf(buf, "\x01\x80", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("wmem") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(8);
		memcpy(buf, "\x10\x00\x01\x00\x01\x80", 6); // wmem 0x1 r1 

		// put test data to write into r1
		set_register(reg, 1, "\x11\x11", REG_SIZE_BYTES);
		
		// should return REG_SIZE_BYTES*3
		asserteq(run_instruction(buf, 0, reg, stack),3*REG_SIZE_BYTES);	

		// test buf[2-4] = 0x1111 	
		asserteq_buf(buf+0x2, "\x11\x11", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("call") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(4);
		memcpy(buf, "\x11\x00\x00\x00", 4); // call 0x0000 

		// should return 0
		asserteq(run_instruction(buf, 0, reg, stack),0);	

		// check that next instruction 0x0002 was pushed onto the stack
		asserteq_buf(stack->mem+(stack->num_elements-1)*REG_SIZE_BYTES, "\x02\x00", REG_SIZE_BYTES);
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("ret") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(4);
		memcpy(buf, "\x11\x00\x00\x00", 4); // call 0x0000  

		// should return 0
		asserteq(run_instruction(buf, 0, reg, stack),0);	

		// check that next instruction 0x0002 was pushed onto the stack
		asserteq_buf(stack->mem+(stack->num_elements-1)*REG_SIZE_BYTES, "\x02\x00", REG_SIZE_BYTES);
		
		// now for the ret, should return back to 0x0002
		memcpy(buf, "\x12\x00", 2);
		asserteq(run_instruction(buf, 0, reg, stack),0x02*REG_SIZE_BYTES);	
		
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}
	it("out") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf,*outbuf;
		size_t alloc_size;
		buf = malloc(4);
		memcpy(buf, "\x13\x00\x65\x00", 4); // out 'A' 

		// open a FILE pointer to our buffer
		out = open_memstream(&outbuf, &alloc_size);
		if(out == NULL){
			printf("error opening memstream\n");
		}
		// should return 2*REG_SIZE_BYTES
		asserteq(run_instruction(buf, 0, reg, stack),2*REG_SIZE_BYTES);			
		asserteq_buf(outbuf, "\x65", 1);

		defer(free(outbuf));
		defer(fclose(out));
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	
	}/*	
	it("in") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf,*inbuf;
		size_t alloc_size;
		buf = malloc(4);
		memcpy(buf, "\x14\x00\x01\x80", 4); // in r1  
		
		// open a FILE pointer to our buffer
		in = open_memstream(&inbuf, &alloc_size);
		if(in == NULL) {
			printf("error opening memstream\n");
		}

		// should return REG_SIZE_BYTES
		in = stdin;
		asserteq(run_instruction(buf, 0, reg, stack),2*REG_SIZE_BYTES);	
		//fprintf(in, "%c", 'A'); // write 'A' to FILE * in
		//fwrite("A", 1, 1, in);
		
		// check that 'A' is now in r1
		

		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	

	}*/	
	it("nop") {
		stack_info * stack = stack_init(INIT_STACK_SIZE);
		registers * reg = malloc(sizeof(struct registers));
		assertneq(stack, NULL);

		// test instruction
		char *buf;
		buf = malloc(4);
		memcpy(buf, "\x15\x00", 2); // nop  

		// should return REG_SIZE_BYTES
		asserteq(run_instruction(buf, 0, reg, stack),REG_SIZE_BYTES);	
		defer(free(buf));
		defer(cleanup_stack(stack));
		defer(free(reg));	

	}	


}

snow_main();


