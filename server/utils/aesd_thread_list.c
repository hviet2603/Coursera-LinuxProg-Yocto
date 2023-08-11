#include <pthread.h>
#include "aesd_thread_list.h"
#include <stdlib.h>

AESDThread *aesd_thread_create(const pthread_attr_t * attr, void* (*thread_func)(void *), void *args)
{
    pthread_t *thread = malloc(sizeof(pthread_t*));
    AESDThread *aesd_thread = malloc(sizeof(AESDThread*));   

    aesd_thread->thread = thread;
    aesd_thread->next = NULL;

    pthread_create(thread, attr, thread_func, args);
    return aesd_thread; 
}

void aesd_thread_join(AESDThread *aesd_thread, void **retval)
{
    pthread_join(*aesd_thread->thread, retval);
}

void aesd_thread_free(AESDThread *aesd_thread)
{
    if (aesd_thread->thread)
    {
        free(aesd_thread->thread);    
    }
    free(aesd_thread);
}

AESDThreadList *aesd_thread_list_init()
{
    AESDThreadList *list = malloc(sizeof(AESDThreadList*));
    list->head = NULL;
    return list;
}

void aesd_thread_list_push(AESDThreadList *list, AESDThread *aesd_thread)
{
    if (list->head == NULL)
    {
        list->head = aesd_thread;
        return;
    }
    
    AESDThread* aesd_thread_p = list->head;
    while(1)
    {
        if (aesd_thread_p->next == NULL)
        {
            aesd_thread_p->next = aesd_thread;
            return;
        }
        aesd_thread_p = aesd_thread_p->next;
    }
}

void aesd_thread_list_free(AESDThreadList *list)
{
    AESDThread* aesd_thread_p = list->head;
    while(1)
    {
        if (aesd_thread_p == NULL)
        {
            break;
        }
        AESDThread* to_free = aesd_thread_p;
        aesd_thread_p = aesd_thread_p->next;
        aesd_thread_free(to_free);
    }
    free(list);    
}
