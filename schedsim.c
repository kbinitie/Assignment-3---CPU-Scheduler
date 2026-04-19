// Brent Ortizo and Kayode Binitie

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

char selected_algorithm[20];
int rr_quantum = 0;

// rr queue state
int rr_queue[MAX_GANTT_TICKS];
int rr_front = 0;
int rr_rear = 0;
int rr_size = 0;
int rr_current = -1;
int rr_seen[MAX_PROCESSES] = {0};

// gantt chart + switch tracking
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

    // skip header row
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

        // reset runtime fields
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

void reset_scheduler_state(void) {
    int i;

    current_time = 0;
    finished_count = 0;
    simulation_done = 0;

    gantt_count = 0;
    context_switches = 0;
    last_scheduled = -1;

    rr_front = 0;
    rr_rear = 0;
    rr_size = 0;
    rr_current = -1;

    for (i = 0; i < process_count; i++) {
        rr_seen[i] = 0;
        processes[i].remaining_burst = processes[i].burst;
        processes[i].first_start_time = -1;
        processes[i].completion_time = -1;
        processes[i].started = 0;
        processes[i].finished = 0;
        processes[i].vruntime = 0;
        processes[i].rr_time_used = 0;
    }
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
                // smaller number = higher priority
                best_index = i;
            }
            else if (processes[i].priority == processes[best_index].priority) {
                // tie break by earlier arrival, then lower index
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

int select_cfs_process(void) {
    int i;
    int best_index = -1;

    for (i = 0; i < process_count; i++) {
        if (processes[i].arrival <= current_time && !processes[i].finished) {
            if (best_index == -1) {
                best_index = i;
            }
            else if (processes[i].vruntime < processes[best_index].vruntime) {
                // pick smallest vruntime
                best_index = i;
            }
            else if (processes[i].vruntime == processes[best_index].vruntime) {
                // tie break by earlier arrival, then lower index
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

void enqueue_rr(int process_index) {
    if (rr_size < MAX_GANTT_TICKS) {
        rr_queue[rr_rear] = process_index;
        rr_rear = (rr_rear + 1) % MAX_GANTT_TICKS;
        rr_size++;
    }
}

int dequeue_rr(void) {
    int process_index = -1;

    if (rr_size > 0) {
        process_index = rr_queue[rr_front];
        rr_front = (rr_front + 1) % MAX_GANTT_TICKS;
        rr_size--;
    }

    return process_index;
}

void add_rr_arrivals(void) {
    int i;

    for (i = 0; i < process_count; i++) {
        if (processes[i].arrival <= current_time &&
            !processes[i].finished &&
            !rr_seen[i] &&
            i != rr_current) {

            enqueue_rr(i);
            rr_seen[i] = 1;
        }
    }
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

void print_metrics(double *avg_wait, double *avg_turn, double *avg_resp) {
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

    *avg_wait = total_waiting / process_count;
    *avg_turn = total_turnaround / process_count;
    *avg_resp = total_response / process_count;

    printf("\nPerformance Metrics:\n");
    printf("Average Waiting Time: %.2f\n", *avg_wait);
    printf("Average Turnaround Time: %.2f\n", *avg_turn);
    printf("Average Response Time: %.2f\n", *avg_resp);
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

            // debug note: commented out for final submission
            /*
            printf("Process %s ran for one tick. Remaining burst: %d\n",
                   p->pid, p->remaining_burst);
            */

            if (p->remaining_burst == 0) {
                p->finished = 1;
                p->completion_time = local_time + 1;
                finished_count++;

                // debug note: commented out for final submission
                /*
                printf("Process %s finished at time %d\n",
                       p->pid, p->completion_time);
                */
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

    // debug note: commented out for final submission
    /*
    printf("\nScheduler started.\n");
    */

    while (1) {
        pthread_mutex_lock(&state_mutex);

        if (finished_count >= process_count) {
            pthread_mutex_unlock(&state_mutex);
            break;
        }

        if (strcmp(selected_algorithm, "priority") == 0) {
            chosen = select_priority_process();
        }
        else if (strcmp(selected_algorithm, "rr") == 0) {
            add_rr_arrivals();

            // if no current rr process, pull next from queue
            if (rr_current == -1 && rr_size > 0) {
                rr_current = dequeue_rr();
            }

            chosen = rr_current;
        }
        else if (strcmp(selected_algorithm, "cfs") == 0) {
            chosen = select_cfs_process();
        }
        else {
            chosen = -1;
        }

        if (chosen == -1) {
            // debug note: commented out for final submission
            /*
            printf("\nScheduler: time %d, IDLE\n", current_time);
            */

            record_gantt_entry("IDLE");

            if (last_scheduled != -1) {
                context_switches++;
            }
            last_scheduled = -1;

            current_time++;
            pthread_mutex_unlock(&state_mutex);
            continue;
        }

        // debug note: commented out for final submission
        /*
        printf("\nScheduler: time %d, running %s\n",
               current_time, processes[chosen].pid);
        */

        record_gantt_entry(processes[chosen].pid);

        // count switch when process changes
        if (last_scheduled != -1 && last_scheduled != chosen) {
            context_switches++;
        }
        last_scheduled = chosen;

        pthread_mutex_unlock(&state_mutex);

        sem_post(&process_sems[processes[chosen].index]);
        sem_wait(&tick_done_sem);

        pthread_mutex_lock(&state_mutex);

        current_time++;

        if (strcmp(selected_algorithm, "rr") == 0) {
            if (processes[chosen].finished) {
                processes[chosen].rr_time_used = 0;
                rr_current = -1;
            }
            else {
                processes[chosen].rr_time_used++;

                // if quantum used up, send process to back of queue
                if (processes[chosen].rr_time_used >= rr_quantum) {
                    enqueue_rr(chosen);
                    processes[chosen].rr_time_used = 0;
                    rr_current = -1;
                }
            }
        }
        else if (strcmp(selected_algorithm, "cfs") == 0) {
            // simplified cfs: add 1 tick to vruntime
            processes[chosen].vruntime += 1;
        }

        pthread_mutex_unlock(&state_mutex);
    }

    pthread_mutex_lock(&state_mutex);
    simulation_done = 1;
    pthread_mutex_unlock(&state_mutex);

    for (i = 0; i < process_count; i++) {
        sem_post(&process_sems[i]);
    }

    // debug note: commented out for final submission
    /*
    printf("\nScheduler finished.\n");
    */

    return NULL;
}

void write_rr_results(const char *task_file,
                      int quantum,
                      double avg_turn,
                      double avg_wait,
                      double avg_resp,
                      int switches) {
    FILE *fp;
    long file_size;

    fp = fopen("rr_results.csv", "a+");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open rr_results.csv\n");
        return;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);

    if (file_size == 0) {
        fprintf(fp, "taskset,quantum,avg_turnaround,avg_waiting,avg_response,context_switches\n");
    }

    fprintf(fp, "%s,%d,%.2f,%.2f,%.2f,%d\n",
            task_file,
            quantum,
            avg_turn,
            avg_wait,
            avg_resp,
            switches);

    fclose(fp);
}

void write_cfs_results(const char *task_file,
                       double avg_turn,
                       double avg_wait,
                       double avg_resp,
                       int switches) {
    FILE *fp;
    long file_size;

    fp = fopen("cfs_results.csv", "a+");
    if (fp == NULL) {
        fprintf(stderr, "Error: could not open cfs_results.csv\n");
        return;
    }

    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);

    if (file_size == 0) {
        fprintf(fp, "taskset,avg_turnaround,avg_waiting,avg_response,context_switches\n");
    }

    fprintf(fp, "%s,%.2f,%.2f,%.2f,%d\n",
            task_file,
            avg_turn,
            avg_wait,
            avg_resp,
            switches);

    fclose(fp);
}

int main(int argc, char *argv[]) {
    int i;
    double avg_wait;
    double avg_turn;
    double avg_resp;

    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Error: incorrect argument count.\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  ./schedsim priority <input_file>\n");
        fprintf(stderr, "  ./schedsim cfs <input_file>\n");
        fprintf(stderr, "  ./schedsim rr <input_file> <time_quantum>\n");
        return 1;
    }

    if (strcmp(argv[1], "priority") != 0 &&
        strcmp(argv[1], "rr") != 0 &&
        strcmp(argv[1], "cfs") != 0) {
        fprintf(stderr, "Error: invalid scheduling algorithm.\n");
        fprintf(stderr, "Valid options are: priority, rr, cfs\n");
        return 1;
    }

    strcpy(selected_algorithm, argv[1]);

    if ((strcmp(selected_algorithm, "priority") == 0 ||
         strcmp(selected_algorithm, "cfs") == 0) && argc != 3) {
        fprintf(stderr, "Error: incorrect argument count for %s.\n", selected_algorithm);
        fprintf(stderr, "Usage: ./schedsim %s <input_file>\n", selected_algorithm);
        return 1;
    }

    if (strcmp(selected_algorithm, "rr") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: incorrect argument count for rr.\n");
            fprintf(stderr, "Usage: ./schedsim rr <input_file> <time_quantum>\n");
            return 1;
        }

        rr_quantum = atoi(argv[3]);
        if (rr_quantum <= 0) {
            fprintf(stderr, "Error: invalid time quantum. Must be a positive integer.\n");
            return 1;
        }
    }

    printf("Algorithm: %s\n", selected_algorithm);
    printf("Input file: %s\n", argv[2]);

    if (strcmp(selected_algorithm, "rr") == 0) {
        printf("Time quantum: %d\n", rr_quantum);
    }

    process_count = load_processes(argv[2], processes);
    if (process_count == -1) {
        return 1;
    }

    if (process_count == 0) {
        fprintf(stderr, "Error: no processes found in input file.\n");
        return 1;
    }

    reset_scheduler_state();

    // debug note: commented out for final submission
    /*
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
    */

    for (i = 0; i < process_count; i++) {
        if (sem_init(&process_sems[i], 0, 0) != 0) {
            fprintf(stderr, "Error: initializing process semaphore %d\n", i);
            return 1;
        }
    }

    if (sem_init(&tick_done_sem, 0, 0) != 0) {
        fprintf(stderr, "Error: initializing tick_done semaphore\n");
        return 1;
    }

    for (i = 0; i < process_count; i++) {
        if (pthread_create(&process_threads[i], NULL, process_runner, &processes[i]) != 0) {
            fprintf(stderr, "Error: creating process thread %d\n", i);
            return 1;
        }
    }

    if (pthread_create(&scheduler_thread, NULL, scheduler_runner, NULL) != 0) {
        fprintf(stderr, "Error: creating scheduler thread\n");
        return 1;
    }

    pthread_join(scheduler_thread, NULL);

    for (i = 0; i < process_count; i++) {
        pthread_join(process_threads[i], NULL);
    }

    print_gantt_chart();
    print_metrics(&avg_wait, &avg_turn, &avg_resp);

    if (strcmp(selected_algorithm, "rr") == 0) {
        write_rr_results(argv[2],
                         rr_quantum,
                         avg_turn,
                         avg_wait,
                         avg_resp,
                         context_switches);
    }
    else if (strcmp(selected_algorithm, "cfs") == 0) {
        write_cfs_results(argv[2],
                          avg_turn,
                          avg_wait,
                          avg_resp,
                          context_switches);
    }

    for (i = 0; i < process_count; i++) {
        sem_destroy(&process_sems[i]);
    }
    sem_destroy(&tick_done_sem);

    pthread_mutex_destroy(&state_mutex);

    return 0;
}