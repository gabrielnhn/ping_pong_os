// Gabriel Nascarella Hishida do Nascimento, GRR 20190361
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"
#include <signal.h>
#include <sys/time.h>
#include <iso646.h>

#define STACKSIZE 64*1024
// task aging for dynamic priority
#define AGING 1
// a milisec.
#define QUANTUM 1000 
#define MAX_TICKS 20

// Global vars
int total_task_count;
int active_user_tasks;
task_t main_task;
task_t* MAIN_TASK = &main_task;
task_t* CURRENT_TASK;
task_t dispatcher;
task_t* DISPATCHER = &dispatcher;
task_t* READY_QUEUE;
task_t* SLEEPING_QUEUE;
bool DONE_CREATING_KERNEL_TASKS = false;
struct sigaction action;
struct itimerval timer;
unsigned int total_tick_count;


void ppos_init()
{
    setvbuf(stdout, 0, _IONBF, 0); // disable buffer
    
    // Create task queue
    total_task_count = 0;
    READY_QUEUE = NULL;

    // Create Dispatcher
    
    task_create(DISPATCHER, (void*)(*dispatcherBody), NULL);
    DISPATCHER->is_user_task = false; // sanity check

    DONE_CREATING_KERNEL_TASKS = true;
    active_user_tasks = 0;

    // Create main task
    task_create(MAIN_TASK, NULL, NULL); // apparently, it works with NULL
    MAIN_TASK->is_user_task = true; // sanity check again
    
    // Define main task as the current one
    CURRENT_TASK = MAIN_TASK;

    // Init alarm signal handler
    action.sa_handler = alarm_handler;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGALRM, &action, 0) < 0)
    {
        perror ("sigaction error!") ;
        exit (1) ;
    }

    // Init timer

    timer.it_value.tv_usec = 1; 
    timer.it_value.tv_sec  = 0;     
    timer.it_interval.tv_usec = QUANTUM;    
    timer.it_interval.tv_sec  = 0;   

    if (setitimer (ITIMER_REAL, &timer, 0) < 0)
    {
        perror ("settimer error!") ;
        exit (1) ;
    }

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
        task->context.uc_stack.ss_sp = stack;
        task->context.uc_stack.ss_size = STACKSIZE;
        task->context.uc_stack.ss_flags = 0;
        task->context.uc_link = 0;
    }
    else
    {
        perror ("Stack error");
        return -1;
    }

    // Set `start_routine` as task entry point
    makecontext (&(task->context), (void*)(*start_routine), 1, arg);


    if (DONE_CREATING_KERNEL_TASKS)
    // only append user tasks to the user tasks queue
    {
        // Append to task queue
        task->is_user_task = true;
        queue_append((queue_t **) &READY_QUEUE, (queue_t*) task);
        active_user_tasks += 1;
    }
    else
        task->is_user_task = false;
    
    // Set attributes
    task->status = READY;
    task_setprio(task, 0);
    // task->when_it_started = total_tick_count;
    // task->processor_time = 0;
    task->last_called = 0;

    return task->id;
} 

int task_switch (task_t *task)
{

    task_t* old = CURRENT_TASK;

    // printf("%d\n", total_tick_count);
    if (old->last_called != 0)
        old->processor_time += total_tick_count - old->last_called;


    task->activations += 1;
    task->last_called = total_tick_count;
    CURRENT_TASK = task;


    return swapcontext(&(old->context), &(task->context));
}

int task_join(task_t* task)
{
    if (task == NULL)
        return -1;

    else if (task->status == DONE)
        return -1;

    CURRENT_TASK->status = SUSPENDED;

    queue_remove((queue_t**)&READY_QUEUE, (queue_t *)CURRENT_TASK);
    queue_append(&task->dependents, (queue_t *)CURRENT_TASK);
    task_yield();

    if (task->status != DONE)
        perror("Something really wrong occured.\n");

    return(task->exit_code);

}

void task_exit (int exit_code)
{
    // free(CURRENT_TASK->context.uc_stack.ss_sp); // Can't do that :(
    CURRENT_TASK->status = DONE;
    CURRENT_TASK->exit_code = exit_code;

    CURRENT_TASK->execution_time = total_tick_count - CURRENT_TASK->when_it_started;

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",
            CURRENT_TASK->id, CURRENT_TASK->execution_time, CURRENT_TASK->processor_time,
            CURRENT_TASK->activations);
    
    if (CURRENT_TASK->is_user_task)
    {
        queue_remove((queue_t**) &READY_QUEUE, (queue_t*) CURRENT_TASK);
        active_user_tasks -= 1;
    }

    if (queue_size(CURRENT_TASK->dependents) > 0)
    {
        queue_t* first = CURRENT_TASK->dependents;
        queue_t* node = first;

        do
        {
            task_t* task = (task_t*) node; 
            node = node->next;

            queue_remove(&CURRENT_TASK->dependents, (queue_t*)task);
            task->status = READY;
            queue_append((queue_t**) &READY_QUEUE, (queue_t*)task);

        } while (queue_size(CURRENT_TASK->dependents) > 0);
        
    }

    // only go back to MAIN_TASK if DISPATCHER is the one exiting.
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

task_t* scheduler()
{
    if (queue_size((queue_t*) READY_QUEUE) == 0)
    {
        return NULL;
    }

    // Find the task whose priority is the smallest
    queue_t* first = (queue_t*) READY_QUEUE;

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
            if (task->dynamic_priority - AGING >= -20) // cant go out of range
                task->dynamic_priority -= AGING; 
        }

        node = node->next;
    } while (node != first);

    // Reset chosen one's dynamic priority
    task_max_priority->dynamic_priority = task_getprio(task_max_priority);

    return task_max_priority;
}

void task_sleep(int t)
{
    CURRENT_TASK->wake_up_time = systime() + t;
    CURRENT_TASK->status = SUSPENDED;

    queue_remove((queue_t**)&READY_QUEUE, (queue_t *)CURRENT_TASK);
    queue_append((queue_t**)&SLEEPING_QUEUE, (queue_t *)CURRENT_TASK);

    task_yield();
}

void wake_up_tasks()
{
    if (queue_size((queue_t*) SLEEPING_QUEUE) == 0)
        // no tasks to awake
        return;

    queue_t* first = (queue_t*) SLEEPING_QUEUE;
    queue_t* node = first;
    int initial_size = queue_size((queue_t*) SLEEPING_QUEUE);
    int counter = 0;

    do 
    {
        task_t* task = (task_t*) node;
        node = node->next;
        if (task->wake_up_time <= systime())
        {
            task->status = READY;

            queue_remove((queue_t**)&SLEEPING_QUEUE, (queue_t *)task);
            queue_append((queue_t**)&READY_QUEUE, (queue_t *)task);
        }
        counter++;
    } while ((queue_size((queue_t*) SLEEPING_QUEUE) > 0) and (counter < initial_size));

}

void print_ready_queue()
// used for debugging purposes.
{
    if (queue_size((queue_t*) READY_QUEUE) == 0)
    {
        fprintf(stderr, "[]"); 
        return;
    }

    queue_t* first = (queue_t*) READY_QUEUE;
    queue_t* node = first;  
    fprintf(stderr, "[");
    do
    {
        task_t* task = (task_t*) node;
        printf(" %d ", task->id);
        node = node->next;
    } while (node != first);
    fprintf(stderr, "]\n");
}

void dispatcherBody(void* arg)
{
    while (active_user_tasks > 0)
    {
        // wakey wakey?
        wake_up_tasks();

        // get next task
        task_t* next_task = scheduler((queue_t**) &READY_QUEUE);

        if (next_task != NULL)
        // is there a task to execute?
        {
            next_task->clock_counter = MAX_TICKS; // reset counter

            task_switch (next_task);
         
            // Deal with task status?
            if (next_task->status == DONE)
                free(next_task->context.uc_stack.ss_sp);

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

void alarm_handler(int signum)
{
    total_tick_count += 1;
    if (CURRENT_TASK->is_user_task)
    {
        CURRENT_TASK->clock_counter -= 1;
 
        if (CURRENT_TASK->clock_counter <= 0)
            task_yield();
    }
}

unsigned int systime()
{
    return total_tick_count;
}