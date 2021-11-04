// Gabriel Nascarella Hishida do Nascimento, GRR 20190361
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"

#define STACKSIZE 64*1024 

// Global vars (declared in ppos_data)
int task_count;
task_t* MAIN_TASK;
task_t* CURRENT_TASK;


void ppos_init()
{
    setvbuf(stdout, 0, _IONBF, 0); // disable buffer

    // Create main task
    task_t main_task;
    MAIN_TASK = &main_task;
    task_create(MAIN_TASK, NULL, NULL); // apparently, it works with NULL

    // Define main task as the current one
    CURRENT_TASK = MAIN_TASK;
    task_count = 1;

}

int task_create(task_t *task, void (*start_routine)(void *),  void *arg)
{
    // Get ID
    task->id = task_count;
    task_count++;

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

    // Set `start_routine` as task entry point, with its argument.
    makecontext (&(task->context), (void*)(*start_routine), 1, arg);

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
    // free(CURRENT_TASK->context.uc_stack.ss_sp); // Can't do that
    task_switch(MAIN_TASK);
}

int task_id()
{
    return CURRENT_TASK->id;
}