.PHONY: clean

CC = gcc
#CROSS_COMPILE = aarch64-none-linux-gnu-

all: default

default: writer.c
	$(CROSS_COMPILE)$(CC) -g -Wall -c writer.c -o writer.o
	$(CROSS_COMPILE)$(CC) -g -Wall writer.o -o writer

clean: writer writer.o
	rm writer.o writer