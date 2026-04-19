#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#include "process.h"

#define MAX_PROCESSES 100
#define MAX_GANTT_TICKS 1000

Process processes[MAX_PROCESSES];
int process_count = 0;

pthread_t process_threads[MAX_PROCESSES];
pthread_t scheduler_thread;

sem_t process_sems[MAX_PROCESSES];
sem_t tick_done_sem;

pthread_mutex_t state_mutex = PTHREAD_MUTEX_INITIALIZER;

int current_time = 0;
int finished_count = 0;
int simulation_done = 0;

/* Gantt chart + context switch tracking */
char gantt_chart[MAX_GANTT_TICKS][MAX_PID_LEN];
int gantt_count = 0;
int context_switches = 0;
int last_scheduled = -1;

int load_processes(const char *filename, Process processes[]) {
    FILE *input_file;
    char header[100];
    int count;

    input_file = fopen(filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: could not open input file %s\n", filename);
        return -1;
    }

    /* Skip the header line */
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

int select_priority_process(void) {
    int i;
    int best_index = -1;

    for (i = 0; i < process_count; i++) {
        if (processes[i].arrival <= current_time && !processes[i].finished) {
            if (best_index == -1) {
                best_index = i;
            }
            else if (processes[i].priority < processes[best_index].priority) {
                best_index = i;
            }
            else if (processes[i].priority == processes[best_index].priority) {
                if (processes[i].arrival < processes[best_index].arrival) {
                    best_index = i;
                }
                else if (processes[i].arrival == processes[best_index].arrival &&
                         processes[i].index < processes[best_index].index) {
                    best_index = i;
                }
            }
        }
    }

    return best_index;
}

void record_gantt_entry(const char *label) {
    if (gantt_count < MAX_GANTT_TICKS) {
        strncpy(gantt_chart[gantt_count], label, MAX_PID_LEN - 1);
        gantt_chart[gantt_count][MAX_PID_LEN - 1] = '\0';
        gantt_count++;
    }
}

void print_gantt_chart(void) {
    int i;

    printf("\nGantt Chart:\n");
    for (i = 0; i < gantt_count; i++) {
        printf("| %s ", gantt_chart[i]);
    }
    printf("|\n");
}

void print_metrics(void) {
    int i;
    double total_waiting = 0.0;
    double total_turnaround = 0.0;
    double total_response = 0.0;

    for (i = 0; i < process_count; i++) {
        int turnaround = processes[i].completion_time - processes[i].arrival;
        int waiting = turnaround - processes[i].burst;
        int response = processes[i].first_start_time - processes[i].arrival;

        total_turnaround += turnaround;
        total_waiting += waiting;
        total_response += response;
    }

    printf("\nPerformance Metrics:\n");
    printf("Average Waiting Time: %.2f\n", total_waiting / process_count);
    printf("Average Turnaround Time: %.2f\n", total_turnaround / process_count);
    printf("Average Response Time: %.2f\n", total_response / process_count);
    printf("Total Context Switches: %d\n", context_switches);
}

void *process_runner(void *arg) {
    Process *p = (Process *)arg;
    int local_time;

    while (1) {
        sem_wait(&process_sems[p->index]);

        pthread_mutex_lock(&state_mutex);

        if (simulation_done) {
            pthread_mutex_unlock(&state_mutex);
            break;
        }

        local_time = current_time;

        if (!p->started) {
            p->started = 1;
            p->first_start_time = local_time;
        }

        if (p->remaining_burst > 0) {
            p->remaining_burst--;

            printf("Process %s ran for one tick. Remaining burst: %d\n",
                   p->pid, p->remaining_burst);

            if (p->remaining_burst == 0) {
                p->finished = 1;
                p->completion_time = local_time + 1;
                finished_count++;

                printf("Process %s finished at time %d\n",
                       p->pid, p->completion_time);
            }
        }

        pthread_mutex_unlock(&state_mutex);

        sem_post(&tick_done_sem);
    }

    return NULL;
}

void *scheduler_runner(void *arg) {
    int chosen;
    int i;

    (void)arg;

    printf("\nScheduler started.\n");

    while (1) {
        pthread_mutex_lock(&state_mutex);

        if (finished_count >= process_count) {
            pthread_mutex_unlock(&state_mutex);
            break;
        }

        chosen = select_priority_process();

        if (chosen == -1) {
            printf("\nScheduler: time %d, IDLE\n", current_time);

            record_gantt_entry("IDLE");

            if (last_scheduled != -1) {
                context_switches++;
            }
            last_scheduled = -1;

            current_time++;
            pthread_mutex_unlock(&state_mutex);
            continue;
        }

        printf("\nScheduler: time %d, running %s\n",
               current_time, processes[chosen].pid);

        record_gantt_entry(processes[chosen].pid);

        if (last_scheduled != -1 && last_scheduled != chosen) {
            context_switches++;
        }
        last_scheduled = chosen;

        pthread_mutex_unlock(&state_mutex);

        sem_post(&process_sems[processes[chosen].index]);
        sem_wait(&tick_done_sem);

        pthread_mutex_lock(&state_mutex);
        current_time++;
        pthread_mutex_unlock(&state_mutex);
    }

    pthread_mutex_lock(&state_mutex);
    simulation_done = 1;
    pthread_mutex_unlock(&state_mutex);

    for (i = 0; i < process_count; i++) {
        sem_post(&process_sems[i]);
    }

    printf("\nScheduler finished.\n");
    return NULL;
}

int main(int argc, char *argv[]) {
    int quantum = 0;
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

    if (strcmp(argv[1], "priority") != 0) {
        fprintf(stderr, "\nThis version only implements priority scheduling so far.\n");
        fprintf(stderr, "Round Robin and CFS will be added next.\n");
        return 1;
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

    print_gantt_chart();
    print_metrics();

    for (i = 0; i < process_count; i++) {
        sem_destroy(&process_sems[i]);
    }
    sem_destroy(&tick_done_sem);

    pthread_mutex_destroy(&state_mutex);

    return 0;
}