//to answer question 3b, I removed the "safe" section in order to make the code cleaner for fixing the deadlock. 


#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define NUM_THREADS 2
#define THREAD_1 0
#define THREAD_2 1

typedef struct
{
    int threadIdx;
} threadParams_t;


pthread_t threads[NUM_THREADS];
threadParams_t threadParams[NUM_THREADS];

struct sched_param nrt_param;

// On the Raspberry Pi, the MUTEX semaphores must be statically initialized
//
// This works on all Linux platforms, but dynamic initialization does not work
// on the R-Pi in particular as of June 2020.
//
pthread_mutex_t rsrcA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rsrcB = PTHREAD_MUTEX_INITIALIZER;

volatile int rsrcACnt=0, rsrcBCnt=0, noWait=0;


void *grabRsrcs(void *threadp)
{
   threadParams_t *threadParams = (threadParams_t *)threadp;
   int threadIdx = threadParams->threadIdx;

   int rc_1, rc_2; //need 2 of these, otherwise the second thread will overwrite it
   
   //random number gen
   srand(time(NULL));

   //random sleep
   struct timespec random_sleep;
   random_sleep.tv_sec = 0;

   if(threadIdx == THREAD_1)
   {
    printf("THREAD 1 grabbing resources\n");

     //I got this trylock code from https://www.ibm.com/docs/en/zvm/7.4.0?topic=descriptions-pthread-mutex-trylock-attempt-lock-mutex-object.  This checks to see if the resource, and if not it is will print a message.  
     //I modified it to not use errno.  Also added the random number generator for random backoff
    rc_1 = pthread_mutex_trylock(&rsrcA);
     if (rc_1 == 0) {
    puts("thread 1 was granted the mutex A");
    } else if (rc_1 == EBUSY) {
    puts("thread 1 was denied access to the mutex A");
    random_sleep.tv_nsec = (rand() % 1000) * 100000;  //depending on the wait time 
    nanosleep(&random_sleep, NULL);
    
    } else {
    fprintf(stderr, "pthread_mutex_trylock() failed: %d\n", rc_1);
    exit(1);
    }

     
    rsrcACnt++;
    if(!noWait) sleep(1);
    printf("THREAD 1 got A, trying for B\n");
    
    //trylock for B
    rc_1 = pthread_mutex_trylock(&rsrcB);

    if (rc_1 == 0) {
    puts("thread 1 was granted the mutex B");
    } else if (rc_1 == EBUSY) {
    puts("thread 1 was denied access to the mutex B");
    random_sleep.tv_nsec = (rand() % 1000) * 10000;  //depending on the wait time, this would need to be more than 1 second in order for
    nanosleep(&random_sleep, NULL);
    pthread_mutex_unlock(&rsrcA);  //unlock A so that thread 2 can get it
    puts("releasing resource A so that thread 2 can get it");
    } else {
    fprintf(stderr, "pthread_mutex_trylock() failed: %d\n", rc_1);
    exit(1);
    }

    //this is typically where the deadlock happens
    rsrcBCnt++;
    printf("THREAD 1 got A and B\n");
    pthread_mutex_unlock(&rsrcB);
    pthread_mutex_unlock(&rsrcA);
    printf("THREAD 1 done\n");
   }
   
   else
   {
    printf("THREAD 2 grabbing resources\n");

    //trylock for B
    rc_2 = pthread_mutex_trylock(&rsrcB);

    if (rc_2 == 0) {
    puts("thread 2 was granted the mutex B");
    } else if (rc_2 == EBUSY) {
    puts("thread 2 was denied access to the mutex B");
    random_sleep.tv_nsec = (rand() % 1000) * 100000;  //depending on the wait time 
    nanosleep(&random_sleep, NULL);
    pthread_mutex_lock(&rsrcB);
    } else {
    fprintf(stderr, "pthread_mutex_trylock() failed: %d\n", rc_2);
    exit(1);
    }

    rsrcBCnt++;
    if(!noWait) sleep(1);
    printf("THREAD 2 got B, trying for A\n");

    //trylock for A
    rc_2 = pthread_mutex_trylock(&rsrcA);

    if (rc_2 == 0) {
    puts("thread 2 was granted the mutex A");
    } else if (rc_2 == EBUSY) {
    puts("thread 2 was denied access to the mutex A");
    random_sleep.tv_nsec = (rand() % 1000) * 100000;  //depending on the wait time 
    nanosleep(&random_sleep, NULL);
    pthread_mutex_lock(&rsrcA);
    } else {
    fprintf(stderr, "pthread_mutex_trylock() failed: %d\n", rc_2);
    exit(1);
    }

    }

    //this is typically where the deadlock happens
     rsrcACnt++;
     printf("THREAD 2 got B and A\n");
     pthread_mutex_unlock(&rsrcA);
     pthread_mutex_unlock(&rsrcB);
     printf("THREAD 2 done\n");
   
     pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
   int rc, safe=0;

   rsrcACnt=0, rsrcBCnt=0, noWait=0;


   printf("Creating thread %d\n", THREAD_1);
   threadParams[THREAD_1].threadIdx=THREAD_1;
   rc = pthread_create(&threads[0], NULL, grabRsrcs, (void *)&threadParams[THREAD_1]);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 1 spawned\n");


   printf("Creating thread %d\n", THREAD_2);
   threadParams[THREAD_2].threadIdx=THREAD_2;
   rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)&threadParams[THREAD_2]);
   if (rc) {printf("ERROR; pthread_create() rc is %d\n", rc); perror(NULL); exit(-1);}
   printf("Thread 2 spawned\n");

   printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);
   printf("will try to join CS threads unless they deadlock\n");

   
   if(pthread_join(threads[0], NULL) == 0)
      printf("Thread 1: %x done\n", (unsigned int)threads[0]);
   else
      perror("Thread 1");
   

   if(pthread_join(threads[1], NULL) == 0)
     printf("Thread 2: %x done\n", (unsigned int)threads[1]);
   else
     perror("Thread 2");

   if(pthread_mutex_destroy(&rsrcA) != 0)
     perror("mutex A destroy");

   if(pthread_mutex_destroy(&rsrcB) != 0)
     perror("mutex B destroy");

   printf("All done\n");

   exit(0);
}
