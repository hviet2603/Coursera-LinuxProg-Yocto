#include "aesd_thread_list.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

AESDThreadList *aesd_thread_list = NULL;
int threads_should_stop = 0;
int is_ping = 0;
int ctr = 0;

void handle_interrupt(int signal) {
    printf("Caught interrupt signal (Ctrl+C). Cleaning up...\n");
    // Perform any cleanup or desired actions here
    if (aesd_thread_list == NULL)
    {
        printf("Thread list is not initialized\n");
        exit(0);
    }

    threads_should_stop = 1;
    AESDThread *aesd_thread = aesd_thread_list->head;
    while(aesd_thread != NULL)
    {
        aesd_thread_join(aesd_thread, NULL);
        aesd_thread = aesd_thread->next;
    }

    aesd_thread_list_free(aesd_thread_list);

    exit(0); // Exit the program gracefully
}

void ping(void)
{
    while(1)
    {
        if (threads_should_stop)
        {
            break;
        }
        if (is_ping == 1)
        {
            ctr++;
            printf("Ping: %d\n", ctr);
            is_ping = 0;
        }
        sleep(1);
    }
}

void pong(void)
{
    while(1)
    {
        if (threads_should_stop)
        {
            break;
        }      
        if (is_ping == 0)
        {
            ctr++;
            printf("Pong: %d\n", ctr);
            is_ping = 1;
        }
        sleep(1);
    }
}

int main()
{
    signal(SIGINT, handle_interrupt);
    
    is_ping = 1;
    aesd_thread_list = aesd_thread_list_init();
    AESDThread *ping_thread = aesd_thread_create(NULL, (void *(*)(void*)) ping, NULL);
    AESDThread *pong_thread = aesd_thread_create(NULL, (void *(*)(void*)) pong, NULL);
    aesd_thread_list_push(aesd_thread_list, ping_thread);
    aesd_thread_list_push(aesd_thread_list, pong_thread);

    while (1)
    {

    }

    return 0;
}
