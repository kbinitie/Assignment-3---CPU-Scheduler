#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcmp() changed since argv[1] is a string, compare strings with strcmp()

#include "process.h"

#define MAX_PROCESSES 100

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

int main(int argc, char *argv[]) {
    int quantum;

    Process processes[MAX_PROCESSES];
    int process_count;
    int i;

    // need at least program name + algorithm + input file
    if (argc < 3 || argc > 4) { // prog requires either 3 or 4 total args
        fprintf(stderr, "Incorrect argument count.\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  ./schedsim priority <input_file>\n");
        fprintf(stderr, "  ./schedsim cfs <input_file>\n");
        fprintf(stderr, "  ./schedsim rr <input_file> <time_quantum>\n");
        return 1;
    }

    // we validate algorithm
    if (strcmp(argv[1], "priority") != 0 &&
        strcmp(argv[1], "rr") != 0 &&
        strcmp(argv[1], "cfs") != 0) {
        fprintf(stderr, "Invalid scheduling algorithm.\n");
        fprintf(stderr, "Valid options are: priority, rr, cfs\n");
        return 1;
    }

    // priority and cfs should have exactly 3 args
    if ((strcmp(argv[1], "priority") == 0 || strcmp(argv[1], "cfs") == 0) && argc != 3) {
        fprintf(stderr, "Incorrect argument count for %s.\n", argv[1]);
        fprintf(stderr, "Usage: ./schedsim %s <input_file>\n", argv[1]);
        return 1;
    }

    // rr should have exactly 4 args
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

    // temp test output
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

    return 0;
}