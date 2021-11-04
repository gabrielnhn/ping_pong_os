// Gabriel Nascarella Hishida do Nascimento, GRR20190361
#include "queue.h"
#include <stdio.h>
#include <stdbool.h>
#include <iso646.h>

int queue_size(queue_t *queue)
{
    if (queue == NULL) // empty queue
        return 0;
    
    queue_t* first = queue;
    
    queue_t* q = first->next; 
    int size = 1;
    
    while(q != first) // it loops.
    {
        size += 1;
        q = q->next;
    }
    return size;
}

void queue_print(char *name, queue_t *queue, void print_elem (void*))
{
    printf("%s: [", name);

    if (queue == NULL)
    {
        printf("]\n");
        return;
    }

    queue_t* first = queue;
    print_elem(first);

    queue_t* q = first->next; 
    while(q != first)
    {
        printf(" ");
        print_elem(q);
        q = q->next;
    }

    printf("]\n");
}

int queue_append (queue_t **queue, queue_t *elem)
{
    // Asserting initial conditions:
    int error = true;
    if (queue == NULL)
    {
        perror("queue.c error: queue does not exist.");
    }
    else if (elem == NULL)
    {
        perror("queue.c error: elem is NULL.");
    }
    else if ((elem->next != NULL) or (elem->prev != NULL))
    {
        perror("queue.c error: elem already belongs to another queue.");
    }
    else
        error = false;

    if(error)
    {
        perror("\n");
        return -1;
    }

    if (*queue == NULL)
    // Queue is empty
    {
        // Single element
        *queue = elem;
        (*queue)->next = elem;
        (*queue)->prev = elem;
        return 0;
    }

    queue_t* first = *queue;
    
    queue_t* q = first; 
    while(q->next != first)
    {
        q = q->next;
    }
    // q is now the last element of the current queue.

    q->next = elem;
    elem->prev = q;
    elem->next = first;
    elem->next->prev = elem;

    return 0;
}

int queue_remove (queue_t **queue, queue_t *elem)
{
    // Asserting initial conditions:

    int error = true;
    if (queue == NULL)
    {
        perror("queue.c error: queue does not exist.");
    }
    else if (*queue == NULL)
    {
        perror("queue.c error: queue is empty.");
    }
    else if (elem == NULL)
    {
        perror("queue.c error: elem is NULL.");
    }
    else
        error = false;

    if(error)
    {
        perror("\n");
        return -1;
    }

    queue_t* first = *queue;
    
    queue_t* q = first; 
    while(q->next != first)
    {
        if (q == elem)
            break;

        q = q->next;
    }

    if (q == elem)
    // Element was found.
    {
        if (q->next == q)
        // Q is the single element of the queue.
        {
            *queue = NULL;

            // Clean the element
            q->prev = NULL;
            q->next = NULL;
            return 0;
        }

        q->prev->next = q->next;
        q->next->prev = q->prev; 


        if (q == first)
            // Queue was empty
            *queue = first->next; 

        // clean element:
        q->prev = NULL;
        q->next = NULL;
        
        return 0;
    }
    
    // Element not in queue:
    perror("queue.c error: Element not in queue.\n");
    return -1;

}