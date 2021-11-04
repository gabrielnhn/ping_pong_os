// Gabriel Nascarella Hishida do Nascimento, GRR 20190361
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

#define STACKSIZE 64*1024 
#define AGING 1

// Global vars (declared in ppos_data)
int total_task_count;
int active_user_tasks;
task_t* MAIN_TASK;
task_t* CURRENT_TASK;
task_t* DISPATCHER;
task_t* QUEUE;
bool DONE_CREATING_KERNEL_TASKS = false;


void ppos_init()
{
    setvbuf(stdout, 0, _IONBF, 0); // disable buffer
    
    // Create task queue
    total_task_count = 0;
    QUEUE = NULL;

    // Create Dispatcher
    task_t dispatcher;
    DISPATCHER = &dispatcher;
    task_create(DISPATCHER, (void*)(*dispatcherBody), NULL);
    DISPATCHER->is_user_task = false;

    // Create main task
    task_t main_task;
    MAIN_TASK = &main_task;
    task_create(MAIN_TASK, NULL, NULL); // apparently, it works with NULL
    MAIN_TASK->is_user_task = false;
    
    // Define main task as the current one
    CURRENT_TASK = MAIN_TASK;

    DONE_CREATING_KERNEL_TASKS = true;
    active_user_tasks = 0;
}

int task_create(task_t *task, void (*start_routine)(void *),  void *arg)
{
    // Get ID
    task->id = total_task_count;
    total_task_count++;

    // Create context;stack
    getcontext(&(task->context));

    char *stack = malloc (STACKSIZE);
    if (stack)
    {
        task->context.uc_stack.ss_sp = stack ;
        task->context.uc_stack.ss_size = STACKSIZE ;
        task->context.uc_stack.ss_flags = 0 ;
        task->context.uc_link = 0 ;
    }
    else
    {
        perror ("Erro na criação da pilha: ") ;
        return -1;
    }

    // Set `start_routine` as task entry point
    makecontext (&(task->context), (void*)(*start_routine), 1, arg);


    if (DONE_CREATING_KERNEL_TASKS)
    // only append user tasks to the user tasks queue
    {
        // Append to task queue
        task->is_user_task = true;
        queue_append((queue_t **) &QUEUE, (queue_t*) task);
        active_user_tasks += 1;
    }
    else
        task->is_user_task = false;
    
    // Set attributes
    task->status = READY;
    task_setprio(task, 0);

    return task->id;
} 

int task_switch (task_t *task)
{
    task_t* old = CURRENT_TASK;
    CURRENT_TASK = task;
    return swapcontext(&(old->context), &(task->context));
}

void task_exit (int exit_code)
{
    // free(CURRENT_TASK->context.uc_stack.ss_sp); // Can't do that :(
    CURRENT_TASK->status = DONE;
    
    if (CURRENT_TASK->is_user_task)
    {
        queue_remove((queue_t**) &QUEUE, (queue_t*) CURRENT_TASK);
        active_user_tasks -= 1;
    }

    // only go back to MAIN_TASK if it DISPATCHER is the one exiting.
    if (CURRENT_TASK != DISPATCHER)
        task_yield();
    else
        task_switch(MAIN_TASK);
}

int task_id()
{
    return CURRENT_TASK->id;
}

void task_yield()
{
    task_switch(DISPATCHER);
}

task_t* scheduler(queue_t** q)
{
    if (queue_size(*q) == 0)
        // No tasks left
        return NULL;

    // Find the task whose priority is the smallest
    queue_t* first = *q;

    queue_t* node = first;

    task_t* task_max_priority = NULL;
    int max_priority = 21; // out of range

    do 
    {
        task_t* task = (task_t*) node;

        if (task->dynamic_priority < max_priority) // reverse scale
        {
            max_priority = task->dynamic_priority;
            task_max_priority = task; 
        }
        node = node->next;

    } while (node != first);

    // task_max_priority is the chosen one.

    // Let us age the other tasks.
    node = first; // sanity check
    
    do
    {
        task_t* task = (task_t*) node;

        if (task != task_max_priority)
        {
            if (task->dynamic_priority > -20) // cant go out of range
                task->dynamic_priority -= AGING; 
        }
        node = node->next;
    } while (node != first);

    // Reset chosen one's dynamic priority
    task_max_priority->dynamic_priority = task_getprio(task_max_priority);

    return task_max_priority;
}

void dispatcherBody(void* arg)
{

    while (active_user_tasks > 0)
    {
        task_t* next_task = scheduler((queue_t**) &QUEUE);

        // escalonador escolheu uma tarefa?      
        if (next_task != NULL)
        {
            // transfere controle para a próxima tarefa
            task_switch (next_task);
         
            // voltando ao dispatcher, trata a tarefa de acordo com seu estado
            // if (next_task->status != DONE)   
            // {
            //     // queue_remove((queue_t**) &QUEUE, (queue_t*) CURRENT_TASK);
            //     // queue_append((queue_t**) &QUEUE, (queue_t*) CURRENT_TASK);
            //     QUEUE = QUEUE->next;
            // }
        }
    }
    task_exit(0);
}

void task_setprio (task_t *task, int prio)
{
    task->static_priority = prio;
    // Also reset dynamic priority, as it was set according to previous static value.
    task->dynamic_priority = prio;
}

int task_getprio (task_t *task)
{
    if (task == NULL)
        return CURRENT_TASK->static_priority;
    else
        return task->static_priority;
}