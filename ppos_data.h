// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Editado por Gabriel Nascarella Hishida do Nascimento, GRR20190361

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h> // biblioteca POSIX de trocas de contexto
#include "queue.h"    // biblioteca de filas genéricas
#include <stdbool.h>
#include <iso646.h>
#include <stdatomic.h>
#include <math.h>

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
    struct task_t *prev, *next; // ponteiros para usar em filas
    int id;                     // identificador da tarefa

    ucontext_t context; // contexto armazenado da tarefa

    bool is_user_task;
    int status; // READY, DONE, ...
    int static_priority;
    int dynamic_priority;
    int clock_counter;

    unsigned int wake_up_time;

    queue_t *dependents;
    int exit_code;

    // Statistics:
    unsigned int when_it_started;
    unsigned int last_called;
    unsigned int execution_time;
    unsigned int processor_time;
    unsigned int activations;
} task_t;

// Possible Task Status values:
#define READY 0
#define DONE 1
#define SUSPENDED 2

#define SEM_VALID 69
#define SEM_INVALID 0
// estrutura que define um semáforo
typedef struct
{
    int counter;
    task_t *queue;
    atomic_flag lock;
    int valid;
    // preencher quando necessário
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
    // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
    // preencher quando necessário
} barrier_t;

typedef struct item_s
{
    struct item_s *prev, *next;
    void *value;
} item_t;

// estrutura que define uma fila de mensagens
typedef struct
{
    semaphore_t slots;
    semaphore_t items;
    semaphore_t access;

    item_t* queue;
    int item_size;
    bool destroyed;
} mqueue_t;


// Headers used in other files

void dispatcherBody(void *arg);

void alarm_handler(int signum);

void print_ready_queue();

int task_create(task_t *task, void (*start_routine)(void *),  void *arg);

void task_yield();

#endif

// Global vars. to be used in other files

extern task_t *CURRENT_TASK;
extern task_t *READY_QUEUE;
extern bool DONE_CREATING_KERNEL_TASKS;