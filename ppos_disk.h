// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.2 -- Julho de 2017

// interface do gerente de disco rígido (block device driver)

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include <ppos_data.h>
#include "disk.h"
// estruturas de dados e rotinas de inicializacao e acesso
// a um dispositivo de entrada/saida orientado a blocos,
// tipicamente um disco rigido.


#define READ DISK_CMD_READ
#define WRITE DISK_CMD_WRITE

typedef struct request_s
{
    struct request_s *prev, *next;

    int command; // READ or WRITE;
    int block;
    void* buffer;

    task_t* task;
} request_t;

typedef struct
{
    semaphore_t disk_access;
    request_t* request_q;
    task_t* task_q;
    bool active;
    bool disk_signal; // sigusr (disk) or task
} disk_t ;


// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) ;

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) ;

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) ;

void sigusr_handler(int signum)ç

#endif
