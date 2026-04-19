#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PID_LEN 20
 
typedef struct {
    char pid[MAX_PID_LEN];   // process name like P1
    int arrival;             // arrival time
    int burst;               // original burst time
    int priority;            // smaller number = higher priority

    int remaining_burst;     // how much burst time is left
    int first_start_time;    // first time the process gets CPU
    int completion_time;     // time when process finishes

    int started;             // 0 = not started yet, 1 = started
    int finished;            // 0 = not finished, 1 = finished

    int vruntime;            // will use for CFS
    int rr_time_used;        // how much of current RR quantum has been used

    int index;               // going to use for thread and semaphores 
} Process;

#endif