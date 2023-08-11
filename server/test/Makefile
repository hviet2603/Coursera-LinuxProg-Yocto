.PHONY: clean

INC_FLAGS = -I ../inc

utils:
	gcc -c $(INC_FLAGS) ../utils/aesd_thread_list.c -lpthread -o utils.o 

thread_list: utils aesd_thread_list_test.c
	gcc $(INC_FLAGS) aesd_thread_list_test.c utils.o -lpthread -o aesd_thread_list_test.o
	gcc $(INC_FLAGS) aesd_thread_list_test.c utils.o -lpthread -o aesd_thread_list_test

all: thread_list

clean:
	rm -rf *.o
# @echo $(shell ls | grep -e test$)
	rm $(shell ls | grep -e test$)