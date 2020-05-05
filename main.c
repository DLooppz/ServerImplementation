#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h> 
#include <assert.h>
#include "dloopzServer.h"

int i1 = 0;
int i2 = 0;


void f1(void *arg){

    i1++;
    printf("This is task1. Done [%d] times\n",i1);
}

void f2(void *arg){

    i2++;
    printf("This is task2. Done [%d] times\n",i2);
}


int main(void){

    int n_workers = 3;
    ProcFunc_t task1,task2;
    task1 = &f1;
    task2 = &f2;

    // Create server, generators and workers
    WorkServer_t *server = serverCreate0(n_workers);

    FakeWorkUnitGen_t *gen1 = generatorCreate(server);
    FakeWorkUnitGen_t *gen2 = generatorCreate(server);
    FakeWorkUnitGen_t *gen3 = generatorCreate(server);
    FakeWorkUnitGen_t *gen4 = generatorCreate(server);

    WorkerThread_t *workers = workersCreate(n_workers,server);

    // Init server, generators and workers 
    serverInit(server, n_workers);
    puts("\n******************\n(SERVER) initiated\n******************\n");

    workersInit(server,n_workers, workers);
    generatorInit(gen1,task1);
    generatorInit(gen2,task1);
    generatorInit(gen3,task2);
    generatorInit(gen4,task2);

    // Run generators and workers
    sleep(15);
	puts("\n******************\n(SERVER) finished\n******************\n");

    serverPrintStats(server);

    // generatorDestroy(gen1);
    // workersDestroy(workers);

    // serverDestroy(server);


    return 0;
}