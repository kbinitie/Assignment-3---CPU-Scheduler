#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strcmp() changed since argv[1] is a string, compare strings with strcmp()

int main(int argc, char *argv[]) {
    int quantum;

    // Need at least program name + algorithm + input file
    if (argc < 3 || argc > 4) { // prog requires either 3 or 4 total args
        fprintf(stderr, "Incorrect argument count.\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  ./schedsim priority <input_file>\n");
        fprintf(stderr, "  ./schedsim cfs <input_file>\n");
        fprintf(stderr, "  ./schedsim rr <input_file> <time_quantum>\n");
        return 1;
    }

    // NOTE: Switched to useing fprintf to print to error stream

    // We validate algorithm
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

    return 0;
}