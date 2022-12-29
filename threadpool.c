#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

/**
 * threadpool.h
 *
 * This file declares the functionality associated with
 * your implementation of a threadpool.
 */

// maximum number of threads allowed in a pool
#define MAXT_IN_POOL 200

/**
 * the pool holds a queue of this structure
 */
typedef struct work_st
{
      int (*routine) (void*);  //the threads process function
      void * arg;  //argument to the function
      struct work_st* next;  
} work_t;


/**
 * The actual pool
 */
typedef struct _threadpool_st 
{
 	int num_threads;	//number of active threads
	int qsize;	        //number in the queue
	pthread_t *threads;	//pointer to threads
	work_t* qhead;		//queue head pointer
	work_t* qtail;		//queue tail pointer
	pthread_mutex_t qlock;		//lock on the queue list
	pthread_cond_t q_not_empty;	//non empty and empty condidtion vairiables
	pthread_cond_t q_empty;
      int shutdown;            //1 if the pool is in distruction process     
      int dont_accept;       //1 if destroy function has begun
} threadpool;


// "dispatch_fn" declares a typed function pointer.  A
// variable of type "dispatch_fn" points to a function
// with the following signature:
// 
//     int dispatch_function(void *arg);

typedef int (*dispatch_fn)(void *);


/**
 * The work function of the thread
 * this function should:
 * 1. lock mutex
 * 2. if the queue is empty, wait
 * 3. take the first element from the queue (work_t)
 * 4. unlock mutex
 * 5. call the thread routine
 *
 */
void* do_work(void* p)
{
	threadpool *pool = (threadpool*)p;
	int exit_val = 100;
	if((pool->dont_accept) == 1) pthread_exit(&exit_val);
	
	while(1)
	{
		work_t *picked_job;
		
		pthread_mutex_lock(&pool->qlock);
		
		while(pool->qhead == pool->qtail)	// Empty queue!
		{
			pthread_cond_wait(&pool->q_not_empty, &pool->qlock); // wait for available job
		}
		
		if((pool->dont_accept) == 1) pthread_exit(&exit_val);
		
		if(pool->qhead != pool->qtail)
		{
			picked_job = pool->qhead;
			pool->qhead = pool->qhead->next;
			pool->qsize--;
		}
		
		pthread_mutex_unlock(&pool->qlock);
		
		picked_job->routine(picked_job->arg);
		
		free(picked_job);
	}
	
	return NULL;
}

/**
 * create_threadpool creates a fixed-sized thread
 * pool.  If the function succeeds, it returns a (non-NULL)
 * "threadpool", else it returns NULL.
 * this function should:
 * 1. input sanity check 
 * 2. initialize the threadpool structure
 * 3. initialized mutex and conditional variables
 * 4. create the threads, the thread init function is do_work and its argument is the initialized threadpool. 
 */
threadpool* create_threadpool(int num_threads_in_pool)
{
	threadpool *pool = malloc(sizeof(threadpool));
	
	pool->num_threads = num_threads_in_pool;
	pool->qsize = 0;
	pool->threads = malloc(sizeof(pthread_t) * num_threads_in_pool);
	pool->qhead = pool->qtail = NULL;
	
	pthread_cond_init(&pool->q_not_empty, NULL);
	pthread_cond_init(&pool->q_empty, NULL);
	pthread_mutex_init(&pool->qlock, NULL);
	
	pool->shutdown = pool->dont_accept = 0;
	
	for(int i = 0; i < num_threads_in_pool; ++i)
	{
		int result = pthread_create(&pool->threads[i], NULL, do_work, pool);
		
		if(result != 0) return NULL;
	}
	
	return pool;
}

/**
 * dispatch enter a "job" of type work_t into the queue.
 * when an available thread takes a job from the queue, it will
 * call the function "dispatch_to_here" with argument "arg".
 * this function should:
 * 1. create and init work_t element
 * 2. lock the mutex
 * 3. add the work_t element to the queue
 * 4. unlock mutex
 *
 */
void dispatch(threadpool* from_me, dispatch_fn dispatch_to_here, void *arg)
{
	// Using mutex to lock resource, to avoid race condition.
	pthread_mutex_lock(&from_me->qlock);
	
	if(((from_me->qsize) < (from_me->num_threads)) && (from_me->dont_accept) != 1)
	{
		work_t *job = malloc(sizeof(work_t));
		
		job->routine = dispatch_to_here;
		job->arg = arg;
		job->next = NULL;
		
		// If queue is empty
		if(from_me->qtail == NULL)
		{
			// NOtify the threads
			pthread_cond_broadcast(&from_me->q_empty);
			
			// assign the created job to the head and tail of the queue.
			from_me->qhead = from_me->qtail = job;
		}
		
		// Queue not empty
		else
		{
			// Notify the threads
			pthread_cond_broadcast(&from_me->q_not_empty);
			
			// Assign the created job to be the qtail next node or job. in the queue
			from_me->qtail->next = job;
			from_me->qtail = job;	// Make the job the tail now.
		}
		
		// Increase the size of the queue.
		from_me->qsize++;
	}
	else
	{
		fprintf(stderr, "Pool is filled up or destruction process has began!.\n");
		exit(1);
	}
	
	// free the resource.
	pthread_mutex_unlock(&from_me->qlock);
}

/**
 * destroy_threadpool kills the threadpool, causing
 * all threads in it to commit suicide, and then
 * frees all the memory associated with the threadpool.
 */
void destroy_threadpool(threadpool* destroy_me)
{
	destroy_me->dont_accept = 1;
	
	pthread_mutex_lock(&destroy_me->qlock);
	
	while((destroy_me->qsize) > 0)
	{
		pthread_cond_wait(&destroy_me->q_empty, &destroy_me->qlock);
	}
	
	pthread_mutex_unlock(&destroy_me->qlock);
	
	for(int i = 0; i < destroy_me->num_threads; ++i)
	{
		pthread_join(destroy_me->threads[i], NULL);
	}
	
	// Free all allocation from the memory heap.
	free(destroy_me->threads);
	
	free(destroy_me);
}


