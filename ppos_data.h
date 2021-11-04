// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Gabriel Nascarella Hishida do Nascimento, GRR20190361

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"		// biblioteca de filas genéricas
#include <stdbool.h>

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next ;		// ponteiros para usar em filas
  int id ;				// identificador da tarefa
  ucontext_t context ;			// contexto armazenado da tarefa
  bool is_user_task;
  int status;

} task_t ;

// Status:
#define READY 0
#define DONE 1

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;


extern task_t* CURRENT_TASK;
extern task_t* MAIN_TASK;
extern int total_task_count;

void dispatcherBody(void* arg);

#endif

