#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h> 
#include <assert.h>
#include "dloopzServer.h"

WorkUnitId jobId = 0;

// Queue functions -------------------------------------------------------------
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

void QueuePut(Queue_t *pQ, WorkUnit_t *job){
    pthread_mutex_lock(&pQ->lock);
    while(pQ->putter - pQ->getter == MAX_ELEMENTS)
        pthread_cond_wait(&pQ->spaceCond, &pQ->lock);
    pQ->elements[pQ->putter++ % MAX_ELEMENTS] = job;
    pthread_cond_signal(&pQ->valuesCond);
    pthread_mutex_unlock(&pQ->lock);
}

WorkUnit_t* QueueGet(Queue_t *pQ){
    WorkUnit_t* ret; 
    pthread_mutex_lock(&pQ->lock);
    while(pQ->putter == pQ->getter)
        pthread_cond_wait(&pQ->valuesCond, &pQ->lock);
    ret = pQ->elements[pQ->getter++ % MAX_ELEMENTS];
    pthread_cond_signal(&pQ->spaceCond);
    pthread_mutex_unlock(&pQ->lock);
    return ret;
}


// ThreadFunctions ---------------------------------
void* fthreadGenerator(void *GeneratorObject){
    /* Function to be executed by a generator thread. Represents the life of a particular generator. */
    
    int i;
    FakeWorkUnitGen_t *thisGenerator = (FakeWorkUnitGen_t *)GeneratorObject;

    // Get this generator ID
    thisGenerator->generator_id = pthread_self();

    printf("(GENERATOR 0x%lx) Generation begins now\n",thisGenerator->generator_id);
    for (i=0;i<thisGenerator->genParams.life_time;i++){
        // Always check if this generator can run (if server didnt stop it) 
        _generatorTryRun(&thisGenerator->sem);

        // Create a new job request (with some logic select which task)
        WorkUnit_t *JobRequest = workUnitCreate(thisGenerator->task);

        // Generate some context: args for task (with some logic in FWUnitGenParams_t)
        JobRequest->context = NULL;

        // Submit to server (updates must be done)
        workUnitSubmit(JobRequest,thisGenerator->serverToGenerate);

        // Simulate generation difficulty. Also gives time to stop generator if its needed
        _generatorUnlock(&thisGenerator->sem);
        sleep(thisGenerator->genParams.interval);        
    }

    // Use iterator to check errors
    // return ((void *)((i+1) == thisGenerator->genParams.life_time));
    return NULL;
}

void* fthreadWorker(void *WorkerObject){

    WorkUnit_t *newJobToDo;
    WorkerThread_t *thisWorker = (WorkerThread_t *)WorkerObject;

    // Get this worker ID
    thisWorker->worker_id = pthread_self();

    printf("(WORKER 0x%lx) Now begins work\n",thisWorker->worker_id);
    while(1){
        // Ask server for a new job to do
        newJobToDo = workerGetJob(thisWorker->server, thisWorker->worker_id);

        // Notify server that job begins (update both job and server stats)
        workUnitBegins(newJobToDo,thisWorker->server,thisWorker->worker_id);

        // Do the job
        newJobToDo->fun(newJobToDo->context);
        printf("(WORKER 0x%lx) Work finished! (WU ID: %d)\n",thisWorker->worker_id,newJobToDo->id);

        // Update stats and delete WorkUnit
        workUnitFinished(newJobToDo, thisWorker->server, thisWorker->worker_id);

        // hardcode time working
        sleep(1);
    }
    return NULL;
}

// Server functions -------------------------------------------------------------
WorkServer_t* serverCreate1(){
    /* Creates a new server with one queue by default */

    // Alloc memory
    WorkServer_t *newServer = malloc(sizeof(WorkServer_t));
    Queue_t *unicQueue = malloc(sizeof(Queue_t));

    // Type of queue management 
    newServer->queue = unicQueue;

    // Set parameters
    newServer->params.queue_type = 1;
    newServer->params.n_workers = 0;
    newServer->params.thread_ids = NULL;

    // Initiate stats
    newServer->stats.job_requests = 0;
    newServer->stats.jobs_done = 0;
    newServer->stats.jobs_inProgress = 0;
    newServer->stats.jobs_mean_time = 0;
    newServer->stats.largest_time = -1;
    newServer->stats.shortest_time = 3600;
    newServer->stats.total_time = 0;

    printf("(SERVER type_1) created\n");
    return newServer;
}

WorkServer_t* serverCreate0(unsigned int n_queues){
    /* Creates a new server with multiple queues (one for each worker to use) */
    
    // Alloc memory
    WorkServer_t *newServer = malloc(sizeof(WorkServer_t));
    assert(newServer);
    Queue_t *queuesArray = malloc(n_queues*sizeof(Queue_t));
    assert(queuesArray);

    // Type of queue management 
    newServer->queue = queuesArray;
    
    // Set parameters
    newServer->params.queue_type = 0;
    newServer->params.n_workers = n_queues;
    newServer->params.thread_ids = malloc(n_queues * sizeof(pthread_t));
    assert(newServer->params.thread_ids);

    // Initiate stats
    newServer->stats.job_requests = 0;
    newServer->stats.jobs_done = 0;
    newServer->stats.jobs_inProgress = 0;
    newServer->stats.jobs_mean_time = 0;
    newServer->stats.largest_time = -1;
    newServer->stats.shortest_time = 3600;
    newServer->stats.total_time = 0;

    printf("(SERVER type_0) created\n");
    return newServer;
}

void serverDestroy(WorkServer_t *server){
    free(server->queue);
    free(server);
}

void serverInit(WorkServer_t *server, int n_workers){
    
    server->params.n_workers = n_workers;
}

void serverPrintParams(WorkServer_t *server){

    printf("******* Server information *******\n");
    if (server->params.queue_type == 1)
        printf("  Server type: Unic queue\n");
    if (server->params.queue_type == 0)
        printf("  Server type: Multiple queues\n");
    
    printf("  Number of worker threads: %d\n",server->params.n_workers);
    printf("**********************************");
}

void serverPrintStats(WorkServer_t *server){

    printf("******* Server stats *******\n");
    printf("  Number of job requests: %d\n",server->stats.job_requests);
    printf("  Number of jobs done: %d\n",server->stats.jobs_done);
    printf("  Number of jobs in progress: %d\n",server->stats.jobs_inProgress);
    printf("  Jobs total time: %d [s]\n",server->stats.total_time);
    printf("  Jobs mean time: %d [s]\n",server->stats.jobs_mean_time);
    printf("  Jobs dead time: %d [s]\n",server->stats.dead_time);
    printf("  Jobs mean dead time: %d [s]\n",server->stats.mean_dead_time);
    printf("  Jobs largest time: %d [s]\n",server->stats.largest_time);
    printf("  Jobs shortest time: %d [s]\n",server->stats.shortest_time);
    printf("****************************\n");
}

void serverUpdateStats(WorkServer_t *server, WorkUnit_t *job, char flag){

    // Job submitted 
    if (flag == 's'){
        server->stats.job_requests++;
    }
    
    // Job in progress (when job begins)
    else if (flag == 'p'){
        server->stats.jobs_inProgress++;
        server->stats.dead_time += job->stats.startProcTime - job->stats.submitTime;
    }

    // Job done
    else if (flag == 'd'){
        server->stats.jobs_done++;
        server->stats.jobs_inProgress--;
        server->stats.total_time += job->stats.endProcTime - job->stats.submitTime;
        server->stats.jobs_mean_time = server->stats.total_time/server->stats.jobs_done;
        server->stats.mean_dead_time = server->stats.dead_time/server->stats.jobs_done;
        
        // Check if jobTime is the shortest or largest time ever
        if ((job->stats.endProcTime - job->stats.submitTime) < server->stats.shortest_time)
            server->stats.shortest_time = job->stats.endProcTime - job->stats.submitTime;
        
        if ((job->stats.endProcTime - job->stats.submitTime) > server->stats.largest_time)
            server->stats.largest_time = job->stats.endProcTime - job->stats.submitTime;
    }

    else {
        printf("Error in flag for serverUpdateStats. Must be s, p or d and get: %c\n",flag);
        exit(1);
    }
}

int _serverGetWorkerThreadIndex(pthread_t threadID, WorkServer_t *server){

    /* Returns index associated with  worker thread ID */
    for (int i=0;i<server->params.n_workers;i++){
        if (threadID == server->params.thread_ids[i])
            return i;
    }
    printf("Error: ThreadID incorrect\n");
    exit(2);
}


// WorkUnit functions -------------------------------------------------------------
WorkUnit_t* workUnitCreate(ProcFunc_t taskToDo){

    WorkUnit_t *newJob = malloc(sizeof(WorkUnit_t));
    assert(newJob);

    jobId ++;
    newJob->id = jobId;
    newJob->fun = taskToDo;
    newJob->context = NULL;
    newJob->stats.submitTime = 0;
    newJob->stats.startProcTime = 0;
    newJob->stats.endProcTime = 0;

    return newJob;
}

void workUnitDestroy(WorkUnit_t *jobToDestroy){
    free(jobToDestroy);
}

void workUnitSubmit(WorkUnit_t *jobToSubmit, WorkServer_t *server){

    // Server type: 0 (many queues). Submit in a random one
    if (server->params.queue_type == 0){

        QueuePut(&server->queue[ rand() % (server->params.n_workers + 1) ],jobToSubmit);
        jobToSubmit->stats.submitTime = time(NULL);
    }

    // Server type: 1 (unic queue)
    if (server->params.queue_type == 1){
        
        QueuePut(server->queue, jobToSubmit);
        jobToSubmit->stats.submitTime = time(NULL);
    }
    
    serverUpdateStats(server, jobToSubmit, 's');
}

void workUnitBegins(WorkUnit_t *jobBegins, WorkServer_t *server, pthread_t workerID){
    
    // Update stats
    jobBegins->stats.startProcTime = time(NULL);
    serverUpdateStats(server,jobBegins,'p');
}

void workUnitFinished(WorkUnit_t *jobDone, WorkServer_t *server, pthread_t workerID){
    
    // Update stats
    jobDone->stats.endProcTime = time(NULL);
    serverUpdateStats(server,jobDone,'d');

    // Destroy WorkUnit
    workUnitDestroy(jobDone);
}


// Generator object -------------------------------------------------------------

void generatorRun(pthread_mutex_t *mutex){
    pthread_mutex_lock(mutex);
}

void generatorStop(pthread_mutex_t *mutex){
    pthread_mutex_unlock(mutex);
}

void _generatorTryRun(pthread_mutex_t *mutex){
    // Used by generator when attempts to produce a WorkUnit
    pthread_mutex_lock(mutex);
}

void _generatorUnlock(pthread_mutex_t *mutex){
    // Used by generator after producing one succesful WorkUnit
    pthread_mutex_unlock(mutex);
}

FakeWorkUnitGen_t* generatorCreate(WorkServer_t *server){

    // Space for new Generator Unit
    FakeWorkUnitGen_t *newGenerator = malloc(sizeof(FakeWorkUnitGen_t));
    assert(newGenerator);

    // Params by default
    newGenerator->genParams.interval = 1;
    newGenerator->genParams.life_time = GEN_MAX_LIFE;
    newGenerator->genParams.rand_seed = rand() % 64;

    // Mutex init
    pthread_mutex_init(&newGenerator->sem, NULL);

    // Associate to one server
    newGenerator->serverToGenerate = server;

    // No function or list of functions until init()
    newGenerator->task = NULL;

    // No ID until init()

    return newGenerator;
}

void generatorInit(FakeWorkUnitGen_t *newGenerator, ProcFunc_t taskToGenerate){

	pthread_t newGenID;
    int err;

    // Add task (or list of possible task, in future) to generate
    newGenerator->task = taskToGenerate;

    // Init thread (generator gets ID in his thread)
    err = pthread_create(&newGenID, NULL, &fthreadGenerator, newGenerator);
    
    // Check if thread is created successfuly
    if (err) {
        printf("Generator thread creation failed : %s\n", strerror(err));
        exit(3);
    }
    // else
    //     printf("Generator thread initiated with ID : 0x%lx\n", newGenID);
}

void generatorDestroy(FakeWorkUnitGen_t *generatorToDestroy){

    free(generatorToDestroy);
}

void generatorSetParams(FakeWorkUnitGen_t *generator, int interval, int life_time, unsigned int rand_seed){
    /* Optionally use this function to change default parameters of generator */
    
    generator->genParams.interval = interval;
    generator->genParams.life_time = life_time;
    generator->genParams.rand_seed = rand_seed;
}

FWUnitGenParams_t* generatorGetParams(FakeWorkUnitGen_t *generator){
    /* The lucky man/girl who decides to use this function must destroy later this params, or pray for a god to free this space... */

    FWUnitGenParams_t *parameters = malloc(sizeof(FWUnitGenParams_t));
    parameters->interval = generator->genParams.interval;
    parameters->life_time = generator->genParams.life_time;
    parameters->rand_seed = generator->genParams.rand_seed;

    return parameters;
}

// Worker object -------------------------------------------------------------

WorkerThread_t* workersCreate(int nWorkers, WorkServer_t *server){
    
    WorkerThread_t *newWorkersArray = malloc(nWorkers * sizeof(WorkerThread_t));
    assert(newWorkersArray);

    for (int i=0;i<nWorkers;i++){
        newWorkersArray[i].params.status = 1;
        newWorkersArray[i].server = server;
    }

    return newWorkersArray;
}

void workersInit(WorkServer_t *server, int nWorkers, WorkerThread_t* workersArray){
    
	pthread_t newWorkerID[nWorkers];
    int i, err;
    srand(time(NULL));

    // Init threads (workers gets their IDs in their threads)
    for(i = 0; i < nWorkers; i++) {
        err = pthread_create(&newWorkerID[i], NULL, &fthreadWorker, &workersArray[i]);
        server->params.thread_ids[i] = newWorkerID[i];

        // Check if thread is created sucessfuly
        if (err) {
            printf("Worker thread creation failed : %s\n", strerror(err));
            exit(3);
        }
    }
}

void workersDestroy(WorkerThread_t *workersArray){
    
    free(workersArray);
}

WorkUnit_t* workerGetJob(WorkServer_t *server, pthread_t worker_id){
    
    WorkUnit_t *jobToStart;

    // Case of unic queue
    if (server->params.queue_type == 1){
        jobToStart = QueueGet(server->queue);
    }

    // Case of many queues (each worker has a designed queue)
    if (server->params.queue_type == 0){
        int worker_idx = _serverGetWorkerThreadIndex(worker_id, server);
        jobToStart = QueueGet(&server->queue[worker_idx]);
    }

    return jobToStart;
}
