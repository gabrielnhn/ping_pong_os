#include "ppos_data.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int sem_create (semaphore_t *s, int value)
{
    atomic_flag_clear(&(s->lock));
    s->valid = SEM_VALID;
    s->counter = value;
    s->queue = NULL;
    return 0;
}

int sem_down (semaphore_t *s)
{
    if ((s == NULL) or (s->valid != SEM_VALID))
        return -1;

    // printf("BRUH\n");

    while (atomic_flag_test_and_set(&(s->lock)) != false);
    
    s->counter -= 1;
    // printf("%d\n", s->counter);

    if (s->counter < 0)
    {
        queue_remove((queue_t**) &READY_QUEUE, (queue_t*) CURRENT_TASK);
        CURRENT_TASK->status = SUSPENDED;
        queue_append((queue_t**) &(s->queue), (queue_t*) CURRENT_TASK);
        atomic_flag_clear(&(s->lock));
        task_yield();
    }
    else
        atomic_flag_clear(&(s->lock));

    return 0;   
}

int sem_up(semaphore_t* s)
{
    if ((s == NULL) or (s->valid != SEM_VALID))
        return -1;
    
    while (atomic_flag_test_and_set(&(s->lock)) != false);

    s->counter++;

    if (s->counter <= 0)
    // Wake up first task
    {
        task_t* first = s->queue;
        queue_remove((queue_t**)&(s->queue), (queue_t *) first);
        first->status = READY;
        queue_append((queue_t**)&READY_QUEUE, (queue_t *) first);
    }

    atomic_flag_clear(&(s->lock));

    return 0;
}

int sem_destroy (semaphore_t *s)
{
    if ((s == NULL) or (s->valid != SEM_VALID))
        return -1;

    while (atomic_flag_test_and_set(&(s->lock)) != false);

    // Wake up every single task

    if (queue_size((queue_t*) s->queue) > 0)
    {
        queue_t* first = (queue_t*) s->queue;
        queue_t* node = first;
        int counter = 0;

        do 
        {
            task_t* task = (task_t*) node;
            node = node->next;
            task->status = READY;

            queue_remove((queue_t**)&s->queue, (queue_t *)task);
            queue_append((queue_t**)&READY_QUEUE, (queue_t *)task);
            
            counter++;
        } while (s->queue != NULL);
        
        // } while ((queue_size((queue_t*) s->queue) > 0) and (counter < initial_size));
    }
    // Destroy
    atomic_flag_clear(&(s->lock));
    s->valid = SEM_INVALID;
    return 0;
}

int mqueue_create (mqueue_t *queue, int max_msgs, int msg_size)
{
    if (queue == NULL)
        return -1;

    sem_create(&(queue->s), max_msgs);
    queue->queue = NULL;
    queue->item_size = msg_size;
    queue->destroyed = false;
    return 0;
}

int mqueue_send (mqueue_t *queue, void *msg)
{
    sem_down(&(queue->s));

    // sanity checks
    if (queue == NULL)
        return -1;

    if (queue->destroyed == true)
        return -1;

    item_t* item = malloc(sizeof(item_t));
    
    // another sanity check
    if (item == NULL)
        return -1;
    
    item->next = NULL;
    item->prev = NULL;

    memcpy(&(item->value), msg, queue->item_size);
    queue_append((queue_t**) &(queue->queue), (queue_t*) item);
    return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg)
{
    sem_up(&(queue->s));
    item_t* first = queue->queue;

    // sanity checks
    if (queue == NULL)
        return -1;
    
    if (queue->queue == NULL)
        return -1;
    
    if (queue->destroyed == true)
        return -1;

    queue_remove((queue_t**) &(queue->queue), (queue_t*) first);
    memcpy(msg, &(first->value), queue->item_size);
    
    free(first);

    return 0;
}

int mqueue_destroy (mqueue_t *queue)
{
    if (queue->destroyed == true)
        return -1;

    queue->destroyed = true;
    sem_destroy(&(queue->s));

    // Destroy every message

    if (queue_size((queue_t*) queue->queue) > 0)
    {
        queue_t* first = (queue_t*) queue->queue;
        queue_t* node = first;
        int counter = 0;

        do 
        {
            item_t* item = (item_t*) node;
            node = node->next;

            queue_remove((queue_t**)&queue->queue, (queue_t *)item);
            free(item);
            
            counter++;
        } while (queue->queue != NULL);
        

    }
    return 0;
}