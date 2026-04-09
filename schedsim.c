#include <stdio.h>
#include <stdlib.h>

int main (int argc, char *argv[]){

  // Command line validation:
  if(argc > 4){
    printf(stderr, "Incorrect argument count\n");
    printf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
   return -1;
  }

  else{
    if(argv[1] != ("priority" || "rr" || "cfs")){
    printf(stderr, "Invalid scheduling algorithm\n");
    printf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
    }

    if(argv[1] == ("priority" || "cfs") && argc != 3){
      printf(stderr, "Incorrect argument count\n");
      printf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file>");
      return -1;
    }
    if(argv[1] == "rr" && argc != 4){
      printf(stderr, "Incorrect argument count\n");
      printf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
      return -1;

      if(atoi(argv[3]) <= 0){
        printf(stderr, "Invalid time quantum\n");
        printf(stderr, "Usage: ./<argv[0] <scheduling algorithm> <input file> <time quantum>");
        return -1;
      }
    }
  }
  return 0;
}
