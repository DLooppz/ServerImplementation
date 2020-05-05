#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h> 
#include <assert.h>
#include "dloopzServer.h"

int main(void){

    int n_workers = 3;

    // Create server, generators and workers
    WorkServer_t *server_t0 = serverCreate0(n_workers);

    FakeWorkUnitGen_t *gen1 = generatorCreate(server_t0);
    FakeWorkUnitGen_t *gen2 = generatorCreate(server_t0);

    WorkerThread_t *workers = workersCreate(n_workers,server_t0);

    // Init Server
    serverInit(server_t0, workers, n_workers);


    generatorDestroy(gen1);
    generatorDestroy(gen2);
    workersDestroy(workers);

    serverDestroy(server_t0);


    return 0;
}