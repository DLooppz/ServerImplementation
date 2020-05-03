#ifndef DLOOPZSERVER_H_   /* Include guard */
#define DLOOPZSERVER_H_

#include <pthread.h>
#include <time.h>
#define MAX_ELEMENTS 128

// Queue object ---------------------------------
typedef struct {
    unsigned int putter,
                 getter;
    pthread_mutex_t lock;
    pthread_cond_t  spaceCond;
    pthread_cond_t  valuesCond;
    WorkUnit_t elements[MAX_ELEMENTS];
} Queue_t; 

// Queue methods
void QueueInit(Queue_t *pQ);
unsigned int QueueNumElements(Queue_t *pQ);
void QueuePut(Queue_t *pQ, int e);
int QueueGet(Queue_t *pQ, int e);


// TaskToDo object ---------------------------------
typedef void (*ProcFunc_t)(void *ctx);


// Server object ----------------------------------
typedef struct {
    int queue_type; /* 1: unic queue ; 0: multiple queues */
    unsigned int n_workers;  /* number of worker threads */
} WorkServerParams;

typedef struct {
    int job_requests;
    int jobs_inProgress;
    int jobs_done;
    int jobs_mean_time;
    int total_time;
    int larger_time;
    int shortest_time;
} WorkServerStats;

typedef struct {
    Queue_t *queue; /* May be one queue or more */
    WorkServerParams params;
    WorkServerStats stats;
} WorkServer_t;

WorkServer_t* serverInit();
WorkServer_t* serverInit(unsigned int n_queues);
void serverDestroy(WorkServer_t *server);
void serverUpdateParams(WorkServer_t *server, int n_workers);
void serverPrintParams(WorkServer_t *server);
void serverPrintStats(WorkServer_t *server);
void serverUpdateStats(WorkServer_t *server, WorkUnit_t *job);


// WorkUnit object ---------------------------------
typedef unsigned int WorkUnitId;

typedef struct {
    time_t submitTime;
    time_t startProcTime;
    time_t endProcTime;
} WorkUnitStat_t;

typedef struct {
    WorkUnitId id;
    ProcFunc_t fun;
    void *context;
    WorkUnitStat_t stats;
} WorkUnit_t;

WorkUnit_t* workUnitCreate(ProcFunc_t taskToDo);
void workUnitDestroy(WorkUnit_t *jobToDestroy);
void workUnitSetParams(WorkUnit_t *jobToSet, void *contextToSet);
void workUnitGetParams(WorkUnit_t *jobToGet, void *contextToGet);
void workUnitSubmit(WorkUnit_t *jobToSubmit, WorkServer_t *server); // Used from Gens
void workUnitBegins(WorkUnit_t *jobBegins, WorkServer_t *server);   // Used from Workers 
void workUnitFinished(WorkUnit_t *jobDone, WorkServer_t *server);   // Used from Workers

// Generators object ---------------------------------
typedef struct {
    int interval;  /* generation time interval in seconds */
    int life_time; /* number of jobs to generate. -1 for eternal life */
    unsigned int rand_seed; 
} FWUnitGenParams_t;

typedef struct {
    pthread_t generator_id;
    FWUnitGenParams_t genParams;
    pthread_mutex_t sem;
    WorkUnit_t *JobRequest;
} FakeWorkUnitGen_t;

FakeWorkUnitGen_t* generatorInit();
void generatorDestroy();
void generatorRun();
void generatorStop();
void generatorSetParams(); 
void generatorGetParams();

// ------------------------------------------------------
// Workers
typedef struct {
    int status;         /* 0: bussy ; 1: free */
} WorkerThreadStatus;

typedef struct {
    pthread_t worker_id;
    WorkUnit_t *jobToDo;
    WorkerThreadStatus status;
    WorkServer_t *server;
} WorkerThread_t;

WorkerThread_t* workersInit(int nWorkers, WorkServer_t *server);        /* Use from main (server) */
void workersDestroy(WorkerThread_t *workersArray);                      /* Use from main (server) */
WorkUnit_t* workerGetJob(WorkServer_t *server);                         /* Use from workers */


#endif 
