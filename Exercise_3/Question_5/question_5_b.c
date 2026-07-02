//this is as well as I could get it to work.  I think the code could be further optimized with better thread start synchronization and better sleep logic

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sched.h>
#include <unistd.h>
#include <math.h>
#include <syslog.h>

// Basic thread information, including run rates
#define NUM_THREADS 2
#define UPDATE_STATE_THREAD 1
#define READ_STATE_THREAD 0
#define UPDATE_RATE_HZ 1
#define READ_RATE_HZ 0.1 // this one will have the higher priority, due to it's higher rate
#define TOTAL_RUN_TIME_SEC 180 // seconds
#define NSEC_PER_MSEC (1000000)
#define UPDATE_SLEEP (NSEC_PER_MSEC * 1000 * UPDATE_RATE_HZ) // 1 Hz update rate
#define READ_SLEEP (NSEC_PER_MSEC * 1000 * READ_RATE_HZ) // 0.1 Hz read rate
#define READ_TIMEOUT 10 // seconds, the read thread will wait for an update if the resource is locked by the update thread

// thread parameters section, including priority
pthread_t read_state_thread, update_state_thread;
pthread_attr_t attr_read_state, attr_update_state;
struct sched_param read_state_params, update_state_params;

int rt_max_prio, rt_min_prio, min; //priorities if needed

pthread_mutex_t rsrc_state = PTHREAD_MUTEX_INITIALIZER; //mutex for protecting the state structure 

typedef struct position_attitude_state
{
    double latitude, longitude, altitude, roll, pitch, yaw;
    struct timespec sample_time;
} POSITION_AND_ATTITUDE_STATE; // format taken from textbook Chapter 2.8

POSITION_AND_ATTITUDE_STATE current_state = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, {0, 0}}; // initialize structure globally

struct timespec start_time; //global start time


// taken from RT-Clock example code, allows calculation of time difference
double d_ftime(struct timespec *fstart, struct timespec *fstop) 
{
    double dfstart = ((double)(fstart->tv_sec) + ((double)(fstart->tv_nsec) / 1000000000.0));
    double dfstop = ((double)(fstop->tv_sec) + ((double)(fstop->tv_nsec) / 1000000000.0));

    return (dfstop - dfstart);
}
 



// function for setting state (to be called by thread 1), runs at 1 Hz.  Needs to pass in the calculated function value 
void *update_state(void *threadID)
{
    struct timespec sleep_time = {0, 0};
    struct timespec sleep_requested = {0, 0};
    struct timespec remaining_time = {0, 0}; 
    int time_sec = 0, rc;
    struct timespec timestamp; //internal place to store each timestamp
    double elapsed = 0; //internal place to store elapsed time for calculations
    
    

    for (; elapsed < TOTAL_RUN_TIME_SEC;) //only run for 180 seconds, time is updated after the sleep
    {
    //in order to avoid having to lock the mutex for the entire calculation, will first get a timestamp, and then perform the calculation with this time
    clock_gettime(CLOCK_MONOTONIC_RAW, &timestamp); // get the current time for the sample time

    elapsed=d_ftime(&start_time, &timestamp); //update elapsed time
    syslog(LOG_INFO, "Elapsed time since start of program: %f seconds", elapsed);

    syslog(LOG_INFO, "Timestamp to be used for calculation of state:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", timestamp.tv_sec, (timestamp.tv_nsec/1000), timestamp.tv_nsec);
    pthread_mutex_lock(&rsrc_state); 
    current_state.latitude = 2*elapsed;
    current_state.longitude = 3*elapsed;
    current_state.altitude = 4*elapsed;
    current_state.roll = sin(elapsed);
    current_state.pitch = cos(elapsed*elapsed);
    current_state.yaw = cos(elapsed);
    current_state.sample_time = timestamp; //update the sample time with the timestamp
    pthread_mutex_unlock(&rsrc_state);

    //sleep logic for rate (pulled from RT-CLOCK example code)
    /* run test for defined seconds */
    sleep_time.tv_sec=1;
    sleep_time.tv_nsec=0;
    sleep_requested.tv_sec=sleep_time.tv_sec;
    sleep_requested.tv_nsec=sleep_time.tv_nsec;

    /* request sleep time and repeat if time remains */
    do 
    {
        if(rc=nanosleep(&sleep_time, &remaining_time) == 0) break;
         
        sleep_time.tv_sec = remaining_time.tv_sec;
        sleep_time.tv_nsec = remaining_time.tv_nsec;
        
    } 
    while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0)));
		    
    
    }
    
    syslog(LOG_INFO, "Program complete, exiting update thread");

    exit(0); //exit the thread when done
}



// function for getting state (to be called by thread 0), runs at .1 Hz
void *read_state(void *threadID)
{
    double elapsed = 0; //internal place to store elapsed time for calculations
    int rc;
    struct timespec sleep_time = {0, 0};
    struct timespec sleep_requested = {0, 0};
    struct timespec remaining_time = {0, 0}; 
    struct timespec timeout = {READ_TIMEOUT, 0}; //timeout for the read thread to wait for the mutex

    for (;elapsed < TOTAL_RUN_TIME_SEC;) //only run for 180 seconds, time is updated after the sleep
    {
    //only need to read and print out
    rc = pthread_mutex_timedlock(&rsrc_state, &timeout); 
    if (rc == 0) {
        syslog(LOG_INFO, "read thread did not time out");
        
    } else if (rc == ETIMEDOUT) {
        syslog(LOG_ERR, "read thread timed out \n");
        exit(1);
    }
    syslog(LOG_INFO, "The current state is: \n\t%f latitude \n\t%f longitude \n\t%f altitude \n\t%f roll \n\t%f pitch \n\t%f yaw \n\t%ld secs, %ld microsecs, %ld nanosecs\n", current_state.latitude, current_state.longitude, current_state.altitude, current_state.roll, current_state.pitch, current_state.yaw, current_state.sample_time.tv_sec, (current_state.sample_time.tv_nsec/1000), current_state.sample_time.tv_nsec);
    elapsed=d_ftime(&start_time, &current_state.sample_time); //update elapsed time based on the sample time of the state
    pthread_mutex_unlock(&rsrc_state);
    
    syslog(LOG_INFO, "Elapsed time since start of program: %f seconds", elapsed);

    //sleep logic for rate (pulled from RT-CLOCK example code)
    /* run test for defined seconds */
    sleep_time.tv_sec=0;
    sleep_time.tv_nsec=READ_SLEEP;
    sleep_requested.tv_sec=sleep_time.tv_sec;
    sleep_requested.tv_nsec=sleep_time.tv_nsec;

    /* request sleep time and repeat if time remains */
    do 
    {
        if(rc=nanosleep(&sleep_time, &remaining_time) == 0) break;
         
        sleep_time.tv_sec = remaining_time.tv_sec;
        sleep_time.tv_nsec = remaining_time.tv_nsec;
        
    } 
    while (((remaining_time.tv_sec > 0) || (remaining_time.tv_nsec > 0)));
		    
    }

    
    syslog(LOG_INFO, "Program complete, exiting reader thread");

    exit(0); //exit the thread when done
}




// main function
void main(void)
{
    int rc;
    openlog("MyCProgram", LOG_PID | LOG_CONS, LOG_USER);  //basic code for starting a syslog 
    syslog(LOG_INFO, "The service has started successfully.");

    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time); //get the start time for the test
    syslog(LOG_INFO, "Start time: %lld.%09ld", (long long)start_time.tv_sec, start_time.tv_nsec);

    //get max and min priorities for these threads.  
    rt_max_prio = sched_get_priority_max(SCHED_FIFO);
    rt_min_prio = sched_get_priority_min(SCHED_FIFO);

    //set specific scheduling for the read_state_thread, code modified from POSIX_MQ_LOOP/posix_mq.c
    rc = pthread_attr_init(&attr_read_state);
    rc = pthread_attr_setinheritsched(&attr_read_state, PTHREAD_EXPLICIT_SCHED);
    rc = pthread_attr_setschedpolicy(&attr_read_state, SCHED_FIFO);
    read_state_params.sched_priority = rt_max_prio; //the read state thread has a higher priority
    pthread_attr_setschedparam(&attr_read_state, &read_state_params);

    //set specific scheduling for the update_state_thread
    rc = pthread_attr_init(&attr_update_state);
    rc = pthread_attr_setinheritsched(&attr_update_state, PTHREAD_EXPLICIT_SCHED);
    rc = pthread_attr_setschedpolicy(&attr_update_state, SCHED_FIFO);
    update_state_params.sched_priority = rt_min_prio; //the update state thread has a lower priority
    pthread_attr_setschedparam(&attr_update_state, &update_state_params);

    pthread_create(&read_state_thread, &attr_read_state, read_state, NULL);
    pthread_create(&update_state_thread, &attr_update_state, update_state, NULL);
    

    //for some reason, the code does not always get to these
    syslog(LOG_INFO, "pthread join update_state_thread"); 
    pthread_join(update_state_thread, NULL);

    syslog(LOG_INFO, "pthread join read_state_thread");
    pthread_join(read_state_thread, NULL);

    

}
