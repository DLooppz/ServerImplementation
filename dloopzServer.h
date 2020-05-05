#ifndef DLOOPZSERVER_H_   /* Include guard */
#define DLOOPZSERVER_H_

#include <pthread.h>
#include <time.h>
#define MAX_ELEMENTS 128
#define GEN_MAX_LIFE 1000


// TaskToDo object ---------------------------------
typedef void (*ProcFunc_t)(void *ctx);

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

// Queue object ---------------------------------
typedef struct {
    unsigned int putter,
                 getter;
    pthread_mutex_t lock;
    pthread_cond_t  spaceCond;
    pthread_cond_t  valuesCond;
    WorkUnit_t elements[MAX_ELEMENTS];
} Queue_t; 

// Server object ----------------------------------
typedef struct {
    int queue_type; /* 1: unic queue ; 0: multiple queues */
    unsigned int n_workers;  /* number of worker threads */
    pthread_t *thread_ids;
} WorkServerParams;

typedef struct {
    int job_requests;
    int jobs_inProgress;
    int jobs_done;
    int jobs_mean_time;
    int dead_time;
    int mean_dead_time;
    int total_time;
    int largest_time;
    int shortest_time;
} WorkServerStats;

typedef struct {
    Queue_t *queue; /* May be one queue or more */
    WorkServerParams params;
    WorkServerStats stats;
} WorkServer_t;

// Generators object ---------------------------------
typedef struct {
    int interval;  /* generation time interval in seconds */
    int life_time; /* number of jobs to generate */
    unsigned int rand_seed; 
} FWUnitGenParams_t;

typedef struct {
    pthread_t generator_id;
    FWUnitGenParams_t genParams;
    pthread_mutex_t sem;
    ProcFunc_t task;
    WorkServer_t *serverToGenerate;
} FakeWorkUnitGen_t;

// Workers obects ---------------------------------
typedef struct {
    int status;         /* 0: bussy ; 1: free */
} WorkerThreadStatus;

typedef struct {
    pthread_t worker_id;
    WorkerThreadStatus params;
    WorkServer_t *server;
} WorkerThread_t;


// Queue methods ---------------------------------
void QueueInit(Queue_t *pQ);
unsigned int QueueNumElements(Queue_t *pQ);
void QueuePut(Queue_t *pQ, WorkUnit_t *job);
WorkUnit_t* QueueGet(Queue_t *pQ);

// ThreadFunctions ---------------------------------
void* fthreadGenerator(void *GeneratorObject);
void* fthreadWorker(void *WorkerObject);

// Worker methods ----------------------------------
WorkServer_t* serverInit1();
WorkServer_t* serverInit0(unsigned int n_queues);
void serverDestroy(WorkServer_t *server);
void serverUpdateParams(WorkServer_t *server, WorkerThread_t *workersArray, int n_workers);
void serverPrintParams(WorkServer_t *server);
void serverPrintStats(WorkServer_t *server);
void serverUpdateStats(WorkServer_t *server, WorkUnit_t *job, char flag);
int _serverGetWorkerThreadIndex(pthread_t threadID, WorkServer_t *server);

// Worker methods ----------------------------------
WorkUnit_t* workUnitCreate(ProcFunc_t taskToDo);
void workUnitDestroy(WorkUnit_t *jobToDestroy);
void workUnitSubmit(WorkUnit_t *jobToSubmit, WorkServer_t *server); // Used from Gens
void workUnitBegins(WorkUnit_t *jobBegins, WorkServer_t *server, pthread_t workerID);
void workUnitFinished(WorkUnit_t *jobDone, WorkServer_t *server, pthread_t workerID);


// Generator methods ----------------------------------
FakeWorkUnitGen_t* generatorCreate(WorkServer_t *server); 
void generatorInit(FakeWorkUnitGen_t *newGenerator, ProcFunc_t taskToGenerate);  
void generatorDestroy(FakeWorkUnitGen_t *generatorToDestroy); 
void generatorSetParams(FakeWorkUnitGen_t *generator, int interval, int life_time, unsigned int rand_seed); 
FWUnitGenParams_t* generatorGetParams(FakeWorkUnitGen_t *generator); 
void generatorRun(pthread_mutex_t *mutex);  /* Used from main (server) */
void generatorStop(pthread_mutex_t *mutex); /* Used from main (server) */
void _generatorTryRun(pthread_mutex_t *mutex); /* Used from generator units */
void _generatorUnlock(pthread_mutex_t *mutex); /* Used from generator units */


// Workers methods---------------------------------
WorkerThread_t* workersCreate(int nWorkers, WorkServer_t *server);      /* Use from main (server) */
void workersInit(int nWorkers, WorkerThread_t* workersArray);           /* Use from main (server) */
void workersDestroy(WorkerThread_t *workersArray);                      /* Use from main (server) */
WorkUnit_t* workerGetJob(WorkServer_t *server, pthread_t worker_id);    /* Use from workers */


#endif 
