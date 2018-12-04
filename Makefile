CC     = gcc
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer -Wno-format-security
ASAN_LIBS = -static-libasan
CFLAGS := -Wall -Werror --std=gnu99 -g3 -DSNOW_ENABLED 

#ARCH := $(shell uname)
#ifneq ($(ARCH),Darwin)
#  LDFLAGS += -lpthread -lssl -lcrypto 
#endif

# default is to build without address sanitizer enabled
all: 

# the noasan version can be used with valgrind
all_noasan: 


test: stack.o registers.c tests/test.o
	$(CC) -o $@ $(CFLAGS) $(ASAN_FLAGS) $^ $(LDFLAGS) $(ASAN_LIBS)
	
test_noasan: stack_noasan.o registers_noasan.o tests/test_noasan.o
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS) 


%_noasan.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $<

%.o : %.c
	$(CC) -c -o $@ $(CFLAGS) $(ASAN_FLAGS) $<

.PHONY: clean

clean:
	rm -fr *.o test test_noasan tests/test.o 

