# SP26 CPSC 380 Programming Assignment 3 – CPU Scheduler

## Contributors
Brent Matthew Ortizo  
Student ID: 2452997  
Email: ortizo@chapman.edu  

Kayode Binitie  
Student ID: 2461327  
Email: binitie@chapman.edu  

---

## Repository

GitHub Repository:  
https://github.com/kbinitie/Assignment-3---CPU-Scheduler/tree/main

## Description
This project implements a **multithreaded CPU scheduling simulator** using **POSIX threads and semaphores**. Each process in the system is represented as its own thread, while a dedicated scheduler thread controls execution in a **tick-based simulation**.

At each time unit, the scheduler selects one process and allows it to execute for exactly one tick. This creates a deterministic simulation of CPU scheduling behavior.

The simulator supports three scheduling algorithms:
- **Preemptive Priority Scheduling**
- **Round Robin (RR) Scheduling**
- **Simplified Completely Fair Scheduler (CFS)**

In addition to implementing scheduling algorithms, this project collects experimental data and applies **machine learning** to predict optimal Round Robin time quantums.

---

## Design Approach

The system follows a **thread-per-process model** with one scheduler thread:

- Each process thread waits on its own semaphore
- The scheduler selects a process each tick
- The scheduler signals that process to run
- The process executes exactly one unit of work
- The process signals completion back to the scheduler

This ensures:
- Controlled execution (one process per tick)
- Deterministic scheduling behavior
- Proper synchronization using semaphores

---

## Scheduling Algorithms

### Preemptive Priority Scheduling
- Lower number = higher priority
- Scheduler always selects the highest-priority ready process
- The scheduler always selects the highest-priority ready process at each tick (preemptive behavior emerges naturally in the simulation)

---

### Round Robin (RR)
- Processes are stored in a circular queue
- Each process is given a fixed time quantum
- If unfinished, it is placed back into the queue
- Tested time quantums: 1, 2, 4, 8, 12, 14, 16, 18, 20, 25, 30

---

### Simplified CFS
- Selects process with the smallest **vruntime**
- Each tick increments vruntime by the amount of CPU time used (1 per tick in this simulation)
- Ensures fair CPU distribution among processes

---

## Workloads Used

### tasks.txt

```
PID Arrival Burst Priority
P1 0 8 3
P2 1 4 1
P3 2 9 4
P4 3 5 2
```

### tasks1.txt

```
PID Arrival Burst Priority
P1 0 2 1
P2 1 3 2
P3 2 1 3
```

### tasks2.txt

```
PID Arrival Burst Priority
P1 0 20 3
P2 2 15 2
P3 4 25 1
```

### tasks3.txt

```
PID Arrival Burst Priority
P1 0 8 3
P2 1 2 1
P3 2 10 4
P4 3 1 2
```

---

## Output

Each execution prints:

- **Gantt Chart**
```
| P1 | P2 | P2 | P2 | …
```

- **Performance Metrics**
  - Average Waiting Time
  - Average Turnaround Time
  - Average Response Time
  - Total Context Switches

Debug output (process logs, scheduler logs, etc.) was **commented out** to match assignment requirements.

---

## Sample Output

### Priority Scheduling (`tasks.txt`)
```
Algorithm: priority
Input file: tasks.txt

Gantt Chart:
| P1 | P2 | P2 | P2 | P2 | P4 | P4 | P4 | P4 | P4 | P1 | P1 | P1 | P1 | P1 | P1 | P1 | P3 | P3 | P3 | P3 | P3 | P3 | P3 | P3 | P3 |

Performance Metrics:
Average Waiting Time: 6.50
Average Turnaround Time: 13.00
Average Response Time: 4.25
Total Context Switches: 4
```

---

### Round Robin Scheduling (`tasks.txt`, q = 4)

```
Algorithm: rr
Input file: tasks.txt
Time quantum: 4

Gantt Chart:
| P1 | P1 | P1 | P1 | P2 | P2 | P2 | P2 | P3 | P3 | P3 | P3 | P4 | P4 | P4 | P4 | P1 | P1 | P1 | P1 | P3 | P3 | P3 | P3 | P4 | P3 |

Performance Metrics:
Average Waiting Time: 11.75
Average Turnaround Time: 18.25
Average Response Time: 4.50
Total Context Switches: 7
```

---

### CFS Scheduling (`tasks.txt`)
```
Algorithm: cfs
Input file: tasks.txt

Gantt Chart:
| P1 | P2 | P3 | P4 | P1 | P2 | P3 | P4 | P1 | P2 | P3 | P4 | P1 | P2 | P3 | P4 | P1 | P3 | P4 | P1 | P3 | P1 | P3 | P1 | P3 | P3 |

Performance Metrics:
Average Waiting Time: 12.75
Average Turnaround Time: 19.25
Average Response Time: 0.00
Total Context Switches: 24
```

---

## Data Analysis Report

The performance of each scheduling algorithm was evaluated using the same workload (`tasks.txt`) to ensure a direct and fair comparison.

### Priority Scheduling
Priority Scheduling produced the best overall performance for this workload. It achieved an average turnaround time of **13.00** and an average waiting time of **6.50**, both of which are significantly lower than the other algorithms. This is because the scheduler always selects the highest-priority process (lower numerical value), allowing shorter or more important tasks to complete earlier.

Additionally, Priority Scheduling resulted in only **4 context switches**, which is the lowest among all algorithms tested. This indicates minimal overhead and efficient CPU usage. However, this approach can lead to starvation in more complex workloads where lower-priority processes may never get scheduled.

---

### Round Robin Scheduling (q = 4)
Round Robin Scheduling with a time quantum of 4 resulted in an average turnaround time of **18.25** and an average waiting time of **11.75**, both higher than Priority Scheduling. The increase is due to frequent preemption, which introduces additional waiting time for processes.

The number of context switches increased to **7**, reflecting the overhead of time slicing. However, Round Robin provides better fairness than Priority Scheduling because all processes are given CPU time in a cyclic order. The response time (**4.50**) is comparable to Priority Scheduling, indicating that processes are still able to start execution relatively quickly.

---

### Completely Fair Scheduling (CFS)
The CFS algorithm achieved the best response time, with an average response time of **0.00**, meaning each process began execution immediately upon arrival. This demonstrates that CFS strongly prioritizes fairness and responsiveness.

However, this fairness comes at a cost. CFS produced the highest average turnaround time (**19.25**) and waiting time (**12.75**), as well as the largest number of context switches (**24**). The frequent switching between processes increases overhead and delays completion times.

---

### Overall Comparison

- **Best Turnaround Time:** Priority Scheduling (13.00)
- **Best Waiting Time:** Priority Scheduling (6.50)
- **Best Response Time:** CFS (0.00)
- **Lowest Context Switching:** Priority Scheduling (4)
- **Most Fair Scheduling:** CFS

These results show that there is no single optimal scheduler for all situations. Priority Scheduling performs best in terms of efficiency, Round Robin balances fairness and responsiveness, and CFS ensures the most equitable CPU distribution at the cost of performance overhead.

## Experimental Data Collection

### Round Robin Results (`rr_results.csv`)

Each run automatically appends a row:
```
taskset,quantum,avg_turnaround,avg_waiting,avg_response,context_switches
```

This file is:
- **Generated automatically**
- **Never manually edited**
- Used as raw experimental data

---

### Observations from RR Data

- Small quantum (q = 1):
  - High context switching
  - Poor performance

- Medium quantum (q = 4–8):
  - Balanced performance

- Large quantum (q ≥ 12):
  - Best turnaround time
  - Behaves similarly to FCFS

---

## Training Dataset Generation

Generated using:
```
python3 build_rr_training.py
```

This script:
- Groups results by workload
- Finds quantum with lowest turnaround time
- Extracts workload features:
  - Number of processes
  - Average burst time
  - Max/min burst
  - Standard deviation
  - Arrival gap

Outputs:
```
rr_training_data.csv
```

---

## Machine Learning Prediction

Run:
```
python3 ml_rr_predict.py rr_training_data.csv <task_file>
```

Example:
```
python3 ml_rr_predict.py rr_training_data.csv tasks.txt
```

Predictions observed:
- tasks.txt → 12
- tasks1.txt → 1
- tasks2.txt → 20
- tasks3.txt → 2

---

## CFS Results (`cfs_results.csv`)

CFS results were collected and appended automatically.

Observations:
- Provides fairness across processes
- Higher turnaround time compared to optimized RR
- Performs better on balanced workloads
- Performs worse on highly skewed burst times

---

## Important Development Note (macOS vs Linux)

While developing this project on macOS, we encountered issues with:

- POSIX semaphores (`sem_init`, `sem_destroy`)
- Deprecated or unsupported behavior on macOS

To resolve this, we switched to a **Docker-based Linux environment**, which provided:

- Full POSIX semaphore support
- Correct pthread + semaphore behavior
- Consistent execution with assignment expectations

All final testing and execution were performed inside Docker.

---

## Compilation
```
gcc schedsim.c -o schedsim -pthread
```

---

## Execution

Priority:
```
./schedsim priority tasks.txt
```

Round Robin:
```
./schedsim rr tasks.txt 4
```

CFS:
```
./schedsim cfs tasks.txt
```

---

## Assumptions

- One process executes per tick
- All processes eventually finish
- Input format is valid
- RR results file is auto-generated and not manually modified

---

## Key Insights

- **RR performance is highly dependent on quantum**
- Small quantum → too many context switches
- Large quantum → behaves like FCFS
- Machine learning successfully predicts good quantum values
- CFS prioritizes fairness over efficiency

---

## Files Included

- `schedsim.c` — scheduler implementation
- `process.h` — process structure
- `build_rr_training.py` — dataset generator
- `ml_rr_predict.py` — ML predictor (provided)
- `rr_results.csv` — RR experimental data
- `rr_training_data.csv` — ML training data
- `cfs_results.csv` — CFS results
- `tasks*.txt` — workload files (includes tasks.txt to tasks3.txt)
- `README.md` — documentation

---

## Collaboration and References

This project was developed by Brent and Kayode with guidance from lecture materials and assignment instructions.

Brent Ortizo and Kayode Binitie collaborated to design the multithreaded CPU scheduling simulator, implement synchronization using pthreads and semaphores, generate experimental data for Round Robin scheduling, and apply machine learning techniques to analyze and predict optimal scheduling parameters.

The following were used as references:
- POSIX Threads (`pthread`)
- POSIX Semaphores (`sem_init`, `sem_wait`, `sem_post`)
- Linux man pages
- Assignment specification

---

## Conclusion

This project demonstrates how scheduling algorithms impact system performance and how empirical data can be used to improve decisions. By combining operating systems concepts with machine learning, we were able to analyze scheduling behavior and predict optimal configurations.

---