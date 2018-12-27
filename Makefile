CC     = gcc
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
ASAN_LIBS = -static-libasan
CFLAGS := -Wall -Werror --std=gnu99 -g3 -DSNOW_ENABLED 

#ARCH := $(shell uname)
#ifneq ($(ARCH),Darwin)
#  LDFLAGS += -lpthread -lssl -lcrypto 
#endif

# default is to build without address sanitizer enabled
all: vm_release test_noasan 

# the debug version is built with address sanitizer
debug: vm_debug test


vm_release: src/stack_noasan.o src/registers_noasan.o src/instructions_noasan.o src/main_noasan.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

vm_debug: src/stack.o src/registers.o src/instructions.o src/main.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS) $(ASAN_LIBS)

test: src/stack.o src/registers.o src/instructions.o tests/test.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS) $(ASAN_LIBS)
	
test_noasan: src/stack_noasan.o src/registers_noasan.o src/instructions_noasan.o tests/test_noasan.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS) 


%_noasan.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $<

.PHONY: clean

clean:
	rm -fr src/*.o test test_noasan vm_release vm_debug tests/test.o 

