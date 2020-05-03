#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h> 
#include "dloopzServer.h"

void QueueInit(Queue_t *pQ){
    pQ->putter = pQ->getter = 0;
    pthread_mutex_init(&pQ->lock, NULL);
    pthread_cond_init(&pQ->spaceCond, NULL);
    pthread_cond_init(&pQ->valuesCond, NULL);
}

unsigned int QueueNumElements(Queue_t *pQ){
    int ret;
    pthread_mutex_lock(&pQ->lock);
    ret = pQ->putter - pQ->getter;
    pthread_mutex_unlock(&pQ->lock);
    return ret;
}

void QueuePut(Queue_t *pQ, int e){
    pthread_mutex_lock(&pQ->lock);
    while(pQ->putter - pQ->getter == MAX_ELEMENTS)
        pthread_cond_wait(&pQ->spaceCond, &pQ->lock);
    pQ->elements[pQ->putter++ % MAX_ELEMENTS] = e;
    pthread_cond_signal(&pQ->valuesCond);
    pthread_mutex_unlock(&pQ->lock);
}

int QueueGet(Queue_t *pQ, int e){
    int ret;
    pthread_mutex_lock(&pQ->lock);
    while(pQ->putter == pQ->getter)
        pthread_cond_wait(&pQ->valuesCond, &pQ->lock);
    ret = pQ->elements[pQ->getter++ % MAX_ELEMENTS];
    pthread_cond_signal(&pQ->spaceCond);
    pthread_mutex_unlock(&pQ->lock);
    return ret;
}

WorkServer_t* serverInit(){
    /* Creates a new server with one queue by default */

    // Alloc memory
    WorkServer_t *newServer = malloc(sizeof(WorkServer_t));
    Queue_t *unicQueue = malloc(sizeof(Queue_t));

    // Type of queue management 
    newServer->queue = unicQueue;

    // Set parameters
    newServer->params.queue_type = 1;
    newServer->params.n_workers = 0;

    // Initiate stats
    newServer->stats.job_requests = 0;
    newServer->stats.jobs_done = 0;
    newServer->stats.jobs_inProgress = 0;
    newServer->stats.jobs_mean_time = 0;
    newServer->stats.larger_time = 0;
    newServer->stats.shortest_time = 0;
    newServer->stats.total_time = 0;

    return newServer;
}

WorkServer_t* serverInit(unsigned int n_queues){
    /* Creates a new server with multiple queues (one for each worker to use) */
    
    // Alloc memory
    WorkServer_t *newServer = malloc(sizeof(WorkServer_t));
    Queue_t *queuesArray = malloc(n_queues*sizeof(Queue_t));

    // Type of queue management 
    newServer->queue = queuesArray;
    
    // Set parameters
    newServer->params.queue_type = 0;
    newServer->params.n_workers = n_queues;

    


}

