.PHONY: clean

CC = gcc
#CROSS_COMPILE = aarch64-none-linux-gnu-

all: default

default: aesdsocket.c
	$(CROSS_COMPILE)$(CC) -g -Wall -c aesdsocket.c -o aesdsocket.o
	$(CROSS_COMPILE)$(CC) -g -Wall aesdsocket.o -o aesdsocket

clean: aesdsocket.o aesdsocket
	rm aesdsocket.o aesdsocket