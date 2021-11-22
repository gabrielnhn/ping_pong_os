#include "disk.h"
#include "queue.h"
#include "ppos_data.h"
#include "ppos.h"
#include "ppos_disk.h"

task_t disk_driver;
task_t* DISK_DRIVER = &disk_driver;

disk_t disk;


void diskDriverBody (void * args)
{
    while (true) 
    {
        // obtém o semáforo de acesso ao disco
        sem_down(&(disk.disk_access));

        // se foi acordado devido a um sinal do disco
        if (disco gerou um sinal)
        {
            // acorda a tarefa cujo pedido foi atendido
        }

        // se o disco estiver livre e houver pedidos de E/S na fila
        //   if (disco_livre && (fila_disco != NULL))
        {
            // escolhe na fila o pedido a ser atendido, usando FCFS
            // solicita ao disco a operação de E/S, usando disk_cmd()
        }

        // libera o semáforo de acesso ao disco

        // suspende a tarefa corrente (retorna ao dispatcher)
        task_yield();
    }
}

int disk_mgr_init (int *numBlocks, int *blockSize)
{
    disk_cmd(DISK_CMD_INIT, 0, NULL);

    // disk driver is not a user task
    DONE_CREATING_KERNEL_TASKS = false; 
    task_create(DISK_DRIVER, diskDriverBody, NULL);
    DONE_CREATING_KERNEL_TASKS = true;
    DISK_DRIVER->status = SUSPENDED;
}