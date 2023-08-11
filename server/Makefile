.PHONY: clean

CC = gcc
INC_FLAGS = -I ./inc
#CROSS_COMPILE = aarch64-none-linux-gnu-

all: default

default: aesdsocket.c
	$(CROSS_COMPILE)$(CC) $(INC_FLAGS) -g -Wall ./utils/aesd_thread_list.c aesdsocket.c -lpthread -o aesdsocket.o
	$(CROSS_COMPILE)$(CC) $(INC_FLAGS) -g -Wall ./utils/aesd_thread_list.c aesdsocket.c -lpthread -o aesdsocket

clean:
	rm -rf aesdsocket
	rm -rf *.o