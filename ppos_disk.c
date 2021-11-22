#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "disk.h"
#include "queue.h"
#include "ppos_data.h"
#include "ppos.h"
#include "ppos_disk.h"


task_t disk_driver;
task_t* DISK_DRIVER = &disk_driver;

disk_t disk;
disk_t* DISK = &disk;

struct sigaction action;

void diskDriverBody (void * args)
{
    request_t* current_req;
    while (true) 
    {
        // obtém o semáforo de acesso ao disco
        sem_down(&(disk.disk_access));

        // se foi acordado devido a um sinal do disco
        if (DISK->disk_signal == true)
        {
            // acorda a tarefa cujo pedido foi atendido
            queue_remove((queue_t**) &(DISK->task_q), (queue_t*) current_req->task);
            current_req->task->status = READY;
            queue_remove((queue_t**) &READY_QUEUE, (queue_t*) current_req->task);

            // remove request from queue
            queue_remove((queue_t**)&(DISK->request_q), (queue_t*) current_req);

            free(current_req);
            DISK->active = false;
            DISK->disk_signal = false;
        }

        // se o disco estiver livre e houver pedidos de E/S na fila
        int status = disk_cmd(DISK_CMD_STATUS, 0, NULL);

        if (not DISK->active && status == DISK_STATUS_IDLE && (DISK->request_q != NULL))
        {
            // escolhe na fila o pedido a ser atendido, usando FCFS
            current_req = DISK->request_q;
            DISK->active = true;

            // solicita ao disco a operação de E/S, usando disk_cmd()
            disk_cmd(current_req->command, current_req->block, current_req->buffer);
        }

        if (DISK->request_q == NULL)
        // make it sleep
        {
            queue_remove((queue_t**) &READY_QUEUE, (queue_t*) DISK_DRIVER);
            DISK_DRIVER->status = SUSPENDED;
        }

        // libera o semáforo de acesso ao disco
        sem_up(&(disk.disk_access));

        // suspende a tarefa corrente (retorna ao dispatcher)
        task_yield();
    }
}

int disk_block_request (int block, void *buffer, int command)
{
    sem_down(&(DISK->disk_access));

    // make request
    request_t* req = malloc(sizeof(request_t));
    if (req == NULL)
        return -1;
    
    req->prev = NULL;
    req->next = NULL;
    req->command = command;
    req->block = block;
    req->buffer = buffer;
    req->task = CURRENT_TASK;

    // add to request queue
    queue_append((queue_t**) &(DISK->request_q), (queue_t*) req);

    // suspend Current task
    queue_remove((queue_t**) &READY_QUEUE, (queue_t*) CURRENT_TASK);
    CURRENT_TASK->status = SUSPENDED;
    queue_append((queue_t**) &(DISK->task_q), (queue_t*) CURRENT_TASK);

    if (DISK_DRIVER->status != READY)
    {
        // wake it up
        DISK_DRIVER->status = READY;
        queue_append((queue_t**) &READY_QUEUE, (queue_t*) DISK_DRIVER);
    }

    sem_up(&(DISK->disk_access));

    task_yield();

    return 0;
}

int disk_block_read (int block, void *buffer)
{
    return disk_block_request(block, buffer, READ);
}

int disk_block_write (int block, void *buffer)
{
    return disk_block_request(block, buffer, WRITE);
}


int disk_mgr_init (int *numBlocks, int *blockSize)
{
    if (disk_cmd(DISK_CMD_INIT, 0, NULL) < 0)
        return -1;

    // handle SIGUSR1 signals.
    action.sa_handler = sigusr_handler;
    sigemptyset (&action.sa_mask) ;
    action.sa_flags = 0 ;
    if (sigaction (SIGUSR1, &action, 0) < 0)
    {
        perror ("sigaction error!") ;
        exit (1) ;
    }

    sem_create(&(DISK->disk_access), 1);
    sem_down(&(DISK->disk_access));

    // disk driver is not a user task

    DISK_DRIVER->status = SUSPENDED;
    
    DISK->request_q = NULL;
    DISK->task_q = NULL;
    DISK->active = false;

    DONE_CREATING_KERNEL_TASKS = false; 
    task_create(DISK_DRIVER, diskDriverBody, NULL);
    DONE_CREATING_KERNEL_TASKS = true;

    sem_up(&(DISK->disk_access));
    return 0;
}


void sigusr_handler(int signum)
{
    if (DISK_DRIVER->status != READY)
    {
        // wake it up
        DISK_DRIVER->status = READY;
        queue_append((queue_t**) &READY_QUEUE, (queue_t*) DISK_DRIVER);
    }
    DISK->disk_signal = true;
}

