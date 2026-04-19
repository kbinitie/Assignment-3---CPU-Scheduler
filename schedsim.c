#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "process.h"

#define MAX_PROCESSES 100

Process processes[MAX_PROCESSES];
int process_count = 0;

pthread_t process_threads[MAX_PROCESSES];
pthread_t scheduler_thread;

sem_t process_sems[MAX_PROCESSES];
sem_t tick_done_sem;

int current_time = 0;
int finished_count = 0;
int simulation_done = 0;

int load_processes(const char *filename, Process processes[]) {
    FILE *input_file;
    char header[100];
    int count;

    input_file = fopen(filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: could not open input file %s\n", filename);
        return -1;
    }

    // skip the header line
    if (fgets(header, sizeof(header), input_file) == NULL) {
        fprintf(stderr, "Error: input file is empty.\n");
        fclose(input_file);
        return -1;
    }

    count = 0;

    while (count < MAX_PROCESSES &&
           fscanf(input_file, "%19s %d %d %d",
                  processes[count].pid,
                  &processes[count].arrival,
                  &processes[count].burst,
                  &processes[count].priority) == 4) {

        processes[count].remaining_burst = processes[count].burst;
        processes[count].first_start_time = -1;
        processes[count].completion_time = -1;
        processes[count].started = 0;
        processes[count].finished = 0;
        processes[count].vruntime = 0;
        processes[count].rr_time_used = 0;
        processes[count].index = count;

        count++;
    }

    fclose(input_file);
    return count;
}

void *process_runner(void *arg) {
    Process *p = (Process *)arg;

    while (1) {
        sem_wait(&process_sems[p->index]);

        if (simulation_done) {
            break;
        }

        if (!p->started) {
            p->started = 1;
            p->first_start_time = current_time;
        }

        if (p->remaining_burst > 0) {
            p->remaining_burst--;
            printf("Process %s ran for one tick. Remaining burst: %d\n",
                   p->pid, p->remaining_burst);

            if (p->remaining_burst == 0) {
                p->finished = 1;
                p->completion_time = current_time + 1;
                finished_count++;
                printf("Process %s finished at time %d\n",
                       p->pid, p->completion_time);
            }
        }

        sem_post(&tick_done_sem);
    }

    return NULL;
}

void *scheduler_runner(void *arg) {
    int i;

    printf("\nScheduler started.\n");

    while (finished_count < process_count) {
        for (i = 0; i < process_count; i++) {
            if (processes[i].arrival <= current_time && !processes[i].finished) {
                printf("\nScheduler: time %d, running %s\n",
                       current_time, processes[i].pid);

                sem_post(&process_sems[processes[i].index]);
                sem_wait(&tick_done_sem);

                current_time++;
                break;
            }
        }

        if (i == process_count) {
            printf("\nScheduler: time %d, IDLE\n", current_time);
            current_time++;
        }
    }

    simulation_done = 1;

    for (i = 0; i < process_count; i++) {
        sem_post(&process_sems[i]);
    }

    printf("\nScheduler finished.\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    int quantum;
    int i;

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Incorrect argument count.\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  ./schedsim priority <input_file>\n");
        fprintf(stderr, "  ./schedsim cfs <input_file>\n");
        fprintf(stderr, "  ./schedsim rr <input_file> <time_quantum>\n");
        return 1;
    }

    if (strcmp(argv[1], "priority") != 0 &&
        strcmp(argv[1], "rr") != 0 &&
        strcmp(argv[1], "cfs") != 0) {
        fprintf(stderr, "Invalid scheduling algorithm.\n");
        fprintf(stderr, "Valid options are: priority, rr, cfs\n");
        return 1;
    }

    if ((strcmp(argv[1], "priority") == 0 || strcmp(argv[1], "cfs") == 0) && argc != 3) {
        fprintf(stderr, "Incorrect argument count for %s.\n", argv[1]);
        fprintf(stderr, "Usage: ./schedsim %s <input_file>\n", argv[1]);
        return 1;
    }

    if (strcmp(argv[1], "rr") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Incorrect argument count for rr.\n");
            fprintf(stderr, "Usage: ./schedsim rr <input_file> <time_quantum>\n");
            return 1;
        }

        quantum = atoi(argv[3]);
        if (quantum <= 0) {
            fprintf(stderr, "Invalid time quantum. Must be a positive integer.\n");
            return 1;
        }
    }

    printf("Algorithm: %s\n", argv[1]);
    printf("Input file: %s\n", argv[2]);

    if (strcmp(argv[1], "rr") == 0) {
        printf("Time quantum: %d\n", quantum);
    }

    process_count = load_processes(argv[2], processes);
    if (process_count == -1) {
        return 1;
    }

    if (process_count == 0) {
        fprintf(stderr, "Error: no processes found in input file.\n");
        return 1;
    }

    printf("\nLoaded Processes:\n");
    for (i = 0; i < process_count; i++) {
        printf("Process %d:\n", i);
        printf("  PID: %s\n", processes[i].pid);
        printf("  Arrival: %d\n", processes[i].arrival);
        printf("  Burst: %d\n", processes[i].burst);
        printf("  Priority: %d\n", processes[i].priority);
        printf("  Remaining Burst: %d\n", processes[i].remaining_burst);
        printf("  Started: %d\n", processes[i].started);
        printf("  Finished: %d\n", processes[i].finished);
        printf("  Index: %d\n", processes[i].index);
    }

    for (i = 0; i < process_count; i++) {
        if (sem_init(&process_sems[i], 0, 0) != 0) {
            fprintf(stderr, "Error initializing process semaphore %d\n", i);
            return 1;
        }
    }

    if (sem_init(&tick_done_sem, 0, 0) != 0) {
        fprintf(stderr, "Error initializing tick_done semaphore\n");
        return 1;
    }

    for (i = 0; i < process_count; i++) {
        if (pthread_create(&process_threads[i], NULL, process_runner, &processes[i]) != 0) {
            fprintf(stderr, "Error creating process thread %d\n", i);
            return 1;
        }
    }

    if (pthread_create(&scheduler_thread, NULL, scheduler_runner, NULL) != 0) {
        fprintf(stderr, "Error creating scheduler thread\n");
        return 1;
    }

    pthread_join(scheduler_thread, NULL);

    for (i = 0; i < process_count; i++) {
        pthread_join(process_threads[i], NULL);
    }

    for (i = 0; i < process_count; i++) {
        sem_destroy(&process_sems[i]);
    }
    sem_destroy(&tick_done_sem);

    return 0;
}