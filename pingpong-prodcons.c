// Producer-Consumer test for the Ping Pong OS.

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include <stdlib.h>
#include "queue.h"

#define MAX_ITEMS 5

typedef struct item_s
{
    struct item_s *prev, *next;
    int value;
} item_t;

item_t* buffer_queue;
task_t p1, p2, p3, c1, c2;
semaphore_t slots_s, items_s, buffer_s;

// corpo da thread A
void Producer (void * arg)
{
    for(int i = 0; i < 200; i++)
    {
        task_sleep (1000);

        // Make item
        item_t* item = malloc(sizeof(item_t));
        item->value = rand() % 100;
        item->next = NULL;
        item->prev = NULL;

        sem_down(&slots_s);

        sem_down(&buffer_s);

        //insere item no buffer
        queue_append((queue_t**) &buffer_queue, (queue_t*) item);
        
        sem_up(&buffer_s);

        sem_up(&items_s);

        printf("%s: made %d\n", (char*) arg, item->value);
    }
    task_exit (0);
}

// corpo da thread B
void Consumer (void * arg)
{
    item_t* item;

    for(int i = 0; i < 200; i++)
    {
        sem_down(&items_s);

        sem_down(&buffer_s);

        item = buffer_queue;
        queue_remove((queue_t**) &buffer_queue, (queue_t*) item);

        sem_up(&buffer_s);

        sem_up(&slots_s);

        printf("%s: consumed %d\n", (char*) arg, item->value);
        free(item);

        task_sleep(1000);
    }
    task_exit (0);
}

int main (int argc, char *argv[])
{
   printf ("main: start\n");

   ppos_init () ;

   // cria semaforos
   sem_create (&buffer_s, 1);
   sem_create (&items_s, 0);
   sem_create (&slots_s, MAX_ITEMS);


   // cria tarefas
   task_create (&p1, Producer, "P1") ;
   task_create (&p2, Producer, "P2") ;
   task_create (&p3, Producer, "P3") ;
   task_create (&c1, Consumer, "                  C1") ;
   task_create (&c2, Consumer, "                  C2") ;


   printf ("main: fim\n") ;
   task_exit (0) ;

   exit (0) ;
}
