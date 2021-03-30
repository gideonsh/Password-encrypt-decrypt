CFLAGS = -Wall -w
FLAGS = -lrt -g3 -lmta_rand -lmta_crypt -lcrypto 
CC = gcc
PROG = Ex2


# SRC will hold an array of all the .c files
SRCS := $(subst ./,,$(shell find . -name "*.c"))

# OBJS will hold an array of the corresponding .out to the .c files
OBJS := $(patsubst %.c,%.out,$(SRCS))

all: $(OBJS)
	
%.out: %.c 
	$(CC) -o $@ $(CFLAGS) $^  $(FLAGS) 

clean:
	rm -vf *.out ${PROG} 

test:
	sudo ./launcher.out 2 -n 30

server:
	sudo ./server 
