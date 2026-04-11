#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct process{
  char pid[20];
  int arrivalTime;
  int burstTime;
  int priority;
} process;


int fileHandler(process *processlist, char *filename);

int main (int argc, char *argv[]){
  
  // Command line validation:
  if(argc > 4){
    fprintf(stderr, "Incorrect argument count\n");
    fprintf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
    return -1;
  }
  
  else{
    if(strcmp(argv[1], "priority") != 0 && strcmp(argv[1], "rr") != 0 && strcmp(argv[1], "cfs") != 0){
      fprintf(stderr, "Invalid scheduling algorithm\n");
      fprintf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
    }
    
    if((strcmp(argv[1], "priority") == 0 || strcmp(argv[1], "cfs") == 0) && argc != 3){
      fprintf(stderr, "Incorrect argument count\n");
      fprintf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file>");
      return -1;
    }
    if(strcmp(argv[1], "rr") == 0 && argc != 4){
      fprintf(stderr, "Incorrect argument count\n");
      fprintf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
      return -1;
      
      if(atoi(argv[3]) <= 0){
        fprintf(stderr, "Invalid time quantum\n");
        fprintf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
        return -1;
      }
    }
  }
  
  process *processlist = NULL; 
  fileHandler(processlist, argv[2]);

  // Printing the processes to verify that they were read correctly
  for(int i = 0; i < sizeof(processlist); i++){
    printf("PID: %s, Arrival Time: %d, Burst Time: %d, Priority: %d\n", processlist[i].pid, processlist[i].arrivalTime, processlist[i].burstTime, processlist[i].priority);
  }

  return 0;
}

// Function to read the input file and store the processes in a dynamic array
int fileHandler(process *processlist, char *filename){
  FILE *file = fopen(filename, "r");
  if(file == NULL){
    fprintf(stderr, "Error opening file\n");
    return -1;
  }
  else{
   process temp;
   // Ignoring the first line of the file
    char buffer[100];
    fgets(buffer, sizeof(buffer), file);

   // Reading the file and storing the processes in a dynamic array
   while (fread(&temp, sizeof(process), 1, file) == 1)
   {
      processlist = realloc(processlist, sizeof(process) * (sizeof(processlist) + 1));
      processlist[sizeof(processlist) - 1] = temp;
   }
    
    fclose(file);
    return 0;
  }
}