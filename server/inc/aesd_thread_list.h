#include <pthread.h>

typedef struct _aesd_thread {
    pthread_t *thread;
    struct _aesd_thread *next;
} AESDThread;


typedef struct _aesd_thread_list {
    AESDThread *head;
} AESDThreadList;

AESDThreadList *aesd_thread_list_init();

void aesd_thread_list_push(AESDThreadList *list, AESDThread *aesd_thread);

void aesd_thread_list_free(AESDThreadList *list);

AESDThread *aesd_thread_create(const pthread_attr_t * attr, void* (*thread_func)(void *), void *args);

void aesd_thread_join(AESDThread *aesd_thread, void **retval);

